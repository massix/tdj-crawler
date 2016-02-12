//
//  main.cpp
//  tdjbgg
//
//  Created by Massimo Gengarelli on 07/02/16.
//  Copyright © 2016 Massimo Gengarelli. All rights reserved.
//

#include <iostream>
#include <vector>
#include <sqlite3.h>
#include <web.h>
#include <config.h>
#include <flate.h>
#include <time.h>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <thread>
#include "connection.h"
#include "request.h"
#include "response.h"
#include "collection.h"
#include "user.h"
#include "game.h"
#include "database.h"

#define BGG_URL "bgg-json.azurewebsites.net"

// Temporary workaround to gracefully stop the server until I don't find a proper solution.
todo::web * server_ptr = nullptr;
bgg_client::connection * connection_ptr = nullptr;

int main(int argc, char *argv[])
{

  todo::config config("./conf/tdj-crawler.conf");
  if (not config.parse_config()) {
    std::cerr << "Unable to parse configuration file\n";
    exit(-1);
  }

	bgg_client::connection connection(BGG_URL);
  connection_ptr = &connection;

  bgg_client::data::database db(config["db_path"]);
  db.create_tables();

  // Retrieve the list of users.
  std::vector<bgg_client::data::user> users_vector;
  struct stat file_stat;
  if (stat(config["users_file"].c_str(), &file_stat) == -1) {
    std::cerr << "Configuration file for users does not exist.\n";
    exit(-1);
  }

  // Shared objects
  bgg_client::data::collection no_expansions;
  bgg_client::data::collection all_games;
  time_t db_last_update = time(0);

  auto update_db_function = [&]()->void {
    std::ifstream users_file(config["users_file"].c_str(), std::ifstream::in | std::ifstream::binary);
    users_vector.clear();

    while (not users_file.eof()) {
      std::string user_info;
      users_file >> user_info;

      if (user_info.empty()) continue;

      std::string bggnick = user_info.substr(0, user_info.find("|"));
      std::string forumnick = user_info.substr(user_info.find("|") + 1, user_info.size());

      users_vector.push_back(bgg_client::data::user(bggnick, forumnick));
    }

    if (not connection.open_connection()) {
      std::cerr << "Unable to connect to BGG\n";
      return;
    }

    users_file.close();

    bgg_client::response response;

    // Everytime we start the server we fetch the latest datas from BGG.
    // This is to avoid to have an empty list in case of DB corruption.
    for (auto & user : users_vector) {
      response.reset();

      std::cout << " --- Fetching bgg user `" << user.getBggNick() << "' `" << user.getForumNick() << "'\n";
      connection.send(bgg_client::request::collection(user.getBggNick()), response);
      db.insert_update_user(user);

      // Force an update of the users' collection after each refresh.
      db.update_user_collection(user, user.getCollection());

      if (response.is_valid()) {
        for (auto & game : response.getGames()) {
          db.user_owns_game(user, game);
          user.accessCollection().push_back(game);

          if (not db.game_exists(game)) {

            // Query BGG to get more details about the game itself
            bgg_client::response r;
            connection.send(bgg_client::request::thing(game.getGameId()), r);

            if (r.is_valid()) {
              std::cout << " => Adding `" << r.getGames()[0].getGameName() << "' to the DB.. ";
              db.insert_update_game(r.getGames()[0]);
              std::cout << "done\n";
            }
          }
        }
      }
    }

    db.all_games(all_games);
    db.all_games_no_expansions(no_expansions);
    db_last_update = time(0);

    std::cout << " --- " << all_games.size() << " total games in DB (expansions included)\n";
    std::cout << " --- " << no_expansions.size() << " total games in DB (no expansions)\n";

    connection.close_connection();
  };

  // Update DB the first time we run the server.
  update_db_function();

  todo::web server(&config);

  // There should be only a single servlet
  auto const & servlet = config.getServlets().front();

  // Register the main servlet.
  todo::web::servlet_t games_servlet = [&](std::string const & p_page, todo::url::cgi_t const & p_cgi, todo::http_request & p_request)->std::string {
    std::string ret;
    std::string content_type = "text/html";
    p_request.m_code = todo::http_request::kOkay;

    Flate * flate = 0;
    flateSetFile(&flate, std::string(servlet["templates"] + "games_template.html").c_str());

    std::vector<std::string> random_greeters =
    {
      "All games owned.",
      "Because games matter.",
      "Simply the best!",
      "We won't bite you.",
      "We accept Italians too!",
      "This is a beta version!",
      "Fun every Friday!"
    };

    std::srand((uint32_t) time(0));
    flateSetVar(flate, "random_greet", random_greeters[(std::rand() % random_greeters.size())].c_str());

    // Users and info in navigator
    for (auto const & user : users_vector) {
      flateSetVar(flate, "user_forumnick", user.getForumNick().c_str());
      flateSetVar(flate, "user_countgames", std::to_string(user.getCollection().size()).c_str());
      flateDumpTableLine(flate, "users_list");
    }

    flateSetVar(flate, "total_games", std::to_string(all_games.size()).c_str());
    flateSetVar(flate, "db_last_update", ctime(&db_last_update));

    for (auto const & g : no_expansions) {
      std::string string_owners;
      flateSetVar(flate, "game_name", g.getGameName().c_str());
      flateSetVar(flate, "game_description", g.getDescription().c_str());
      flateSetVar(flate, "game_thumbnail", g.getThumbnailUrl().c_str());
      std::string game_url = "http://boardgamegeek.com/boardgame/" + std::to_string(g.getGameId());
      flateSetVar(flate, "game_url", game_url.c_str());
      std::string game_authors;

      for (std::string const & author : g.getAuthors()) {
        game_authors += author + ", ";
      }
      game_authors = game_authors.substr(0, game_authors.find_last_of((',')));

      flateSetVar(flate, "game_authors", game_authors.c_str());
      flateSetVar(flate, "game_min_players", std::to_string(g.getMinPlayers()).c_str());
      flateSetVar(flate, "game_max_players", std::to_string(g.getMaxPlayers()).c_str());
      flateSetVar(flate, "game_playtime", std::to_string(g.getPlayingTime()).c_str());

      std::vector<bgg_client::data::user> owners;
      db.users_for_game(owners, g);

      for (auto const & user : owners) {
        std::string bgg_url = "http://boardgamegeek.com/user/" + user.getBggNick();
        string_owners += user.getForumNick() + " (" +
          "<a href=\"" + bgg_url + "\">" +
          user.getBggNick() + "</a>), ";
      }

      // Remove last ","
      string_owners = string_owners.substr(0, string_owners.find_last_of(","));
      flateSetVar(flate, "game_owners", string_owners.c_str());

      // Get expansions
      bgg_client::data::collection expansions;
      db.expansions_for_game(g, expansions);
      flateSetVar(flate, "has_expansions", expansions.empty() ? "none" : "default");
      flateSetVar(flate, "expansions_total", std::to_string(expansions.size()).c_str());

      for (auto const & exp : expansions) {
        flateSetVar(flate, "expansion_name", exp.getGameName().c_str());
        flateSetVar(flate, "expansion_thumbnail", exp.getThumbnailUrl().c_str());
        flateSetVar(flate, "expansion_description", exp.getDescription().c_str());

        std::vector<bgg_client::data::user> exp_owners;
        db.users_for_game(exp_owners, exp);
        std::string exp_owners_string;

        for (auto const & user : exp_owners) {
          std::string bgg_url = "http://boardgamegeek.com/user/" + user.getBggNick();
          exp_owners_string += user.getForumNick() + " (" +
            "<a href=\"" + bgg_url + "\">" +
            user.getBggNick() + "</a>), ";
        }

        // Remove last ","
        exp_owners_string = exp_owners_string.substr(0, exp_owners_string.find_last_of(","));
        flateSetVar(flate, "expansion_owners", exp_owners_string.c_str());

        flateDumpTableLine(flate, "game_expansions");
      }

      flateDumpTableLine(flate, "games_accordion");
    }

    char * buffer = flatePage(flate);
    ret = std::string(buffer);
    flateFreeMem(flate);

    delete buffer;
    return ret;
  };

  server.insert(servlet["address"], games_servlet);
  server_ptr = &server;

  // Register signal handler as a lambda function.
  auto signal_handler = [](int signal)->void {
    std::cout << " -- Handler called, gracefully stopping server --\n";
    if (server_ptr != nullptr) server_ptr->stop();
  };

  signal(SIGINT, signal_handler);
  signal(SIGABRT, signal_handler);
  signal(SIGKILL, signal_handler);

  signal(SIGPIPE, [](int signal){
    std::cerr << " --- Caught SIGPIPE, update failed, reopening connection\n";
    connection_ptr->close_connection();
    connection_ptr->open_connection();
  });

  bool server_running(true);

  // Register the background thread responsible of regularly updating the DB
  std::thread updater([&]() {
    uint32_t timeout = std::atoi(config["update_db_timeout"].c_str());
    std::cout << " --- Timeout BGG for updater thread: " << timeout << std::endl;
    uint32_t slept_time(0);

    while (server_running) {
      sleep(1);

      // We check every second in order to ensure a proper shutdown of the service
      // When needed.
      if (++slept_time >= timeout) {
        update_db_function();
        slept_time = 0;
      }
    }

  });

  std::cout << " --- Server running on port " << config["server_web_port"] << "\n";
  server.run();
  server_running = false;

  updater.join();

  std::cout << "Gracefully stopped server\n";
  return 0;
}
