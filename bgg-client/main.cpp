//
//           DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//                   Version 2, December 2004
//
// Copyright (C) 2016 Massimo Gengarelli <massimo.gengarelli@gmail.com>
//
// Everyone is permitted to copy and distribute verbatim or modified
// copies of this license document, and changing it is allowed as long
// as the name is changed.
//
//            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//
//  0. You just DO WHAT THE FUCK YOU WANT TO.
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
#include <map>
#include <mutex>
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

// Force an update of the base if the following var is true.
bool force_base_update(false);

// Mutex for updater and clients
std::mutex synchro;

// True if the DB is currently updating
bool db_updating(false);

// Decode user's nickname
std::string decode(std::string const & nick) {
  std::string ret;

  bool decoding(false);
  char buffer[10];
  int last_index(0);

  for (auto const & c : nick) {
    if (c == '%') {
      decoding = true;
      last_index = 0;
      bzero(buffer, 10);
    }
    else if (c >= '0' and c <= '9' and decoding) {
      buffer[last_index] = c;
      last_index++;
    }
    else if (decoding) {
      decoding = false;
      ret += (char) strtol(buffer, nullptr, 16);
      ret += c;
    }
    else {
      ret += c;
    }
  }

  return ret;
}

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

  // Map for games, each entry has a list of owners and expansions.
  struct db_representation {
    std::vector<bgg_client::data::user> owners;
    std::vector<bgg_client::data::user> wants_to_play;
    bgg_client::data::collection expansions;
  };

  std::map<bgg_client::data::game, db_representation> in_memory_db;
  time_t db_last_update = time(0);

  auto update_db_function = [&]()->void {
    std::ifstream users_file(config["users_file"].c_str(), std::ifstream::in | std::ifstream::binary);

    synchro.lock();
    db_updating = true;
    users_vector.clear();
    all_games.clear();
    no_expansions.clear();

    // Clear In memory DB.
    for (auto & repr : in_memory_db) {
      repr.second.wants_to_play.clear();
      repr.second.owners.clear();
      repr.second.expansions.clear();
    }

    in_memory_db.clear();

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

      std::cout << " --- Fetching bgg user `" << decode(user.getBggNick()) << "' `" << user.getForumNick() << "'\n";
      connection.send(bgg_client::request::collection(user.getBggNick()), response);
      db.insert_update_user(user);

      // Force an update of the users' collection after each refresh.
      user.accessCollection().clear();
      user.accessWantsToPlay().clear();

      for (auto & game : response.getGames())
        user.accessCollection().push_back(game);
      for (auto & game : response.getWantsToPlay())
        user.accessWantsToPlay().push_back(game);

      db.update_user_collection(user, user.getCollection());
      db.update_user_wants(user, user.getWantsToPlay());

      if (response.is_valid()) {
        for (auto & game : response.getGames()) {
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

    // Purge games without owners.
    for (auto const & game : all_games) {
      std::vector<bgg_client::data::user> owners;
      db.users_for_game(owners, game);
      if (owners.empty()) {
        std::cout << " => Purging game `" << game.getGameName() << "' from DB.\n";
        db.delete_game(game);
      }
    }

    // Re-reset DB.
    all_games.clear();
    no_expansions.clear();
    db.all_games(all_games);
    db.all_games_no_expansions(no_expansions);

    // Fill objects
    for (auto const & game : all_games) {
      bgg_client::data::collection expansions;
      std::vector<bgg_client::data::user> owners;
      std::vector<bgg_client::data::user> wants;
      db.expansions_for_game(game, expansions);
      db.users_for_game(owners, game);
      db.wants_for_game(wants, game);

      in_memory_db[game].owners = owners;
      in_memory_db[game].expansions = expansions;
      in_memory_db[game].wants_to_play = wants;
    }

    // Fill the game objects in users' collections.
    for (auto & user : users_vector) {
      for (auto & game : user.accessCollection()) {
        db.game_by_id(game.getGameId(), game);
      }

      for (auto & game : user.accessWantsToPlay()) {
        db.game_by_id(game.getGameId(), game);
      }
    }

    db_updating = false;
    synchro.unlock();
    std::cout << " --- " << all_games.size() << " total games in DB (expansions included)\n";
    std::cout << " --- " << no_expansions.size() << " total games in DB (no expansions)\n";
    db_last_update = time(0);

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
    p_request["Content-Type"] = content_type;

    Flate * flate = 0;
    flateSetFile(&flate, std::string(servlet["templates"] + "games_template.html").c_str());

    std::vector<std::string> random_greeters =
    {
      "Fun every Friday!"
    };

    std::srand((uint32_t) time(0));
    flateSetVar(flate, "random_greet", random_greeters[(std::rand() % random_greeters.size())].c_str());

    if (db_updating) {
      flateSetVar(flate, "db_updating", "default");
      char * buffer = flatePage(flate);
      ret = std::string(buffer);
      flateFreeMem(flate);

      delete buffer;
      return ret;
    }

    flateSetVar(flate, "db_updating", "none");

    synchro.lock();
    // Users and info in navigator
    for (auto const & user : users_vector) {
      flateSetVar(flate, "user_forumnick", user.getForumNick().c_str());
      flateSetVar(flate, "user_bggnick", user.getBggNick().c_str());
      flateSetVar(flate, "user_countgames", std::to_string(user.getCollection().size()).c_str());
      flateDumpTableLine(flate, "users_list");
    }

    flateSetVar(flate, "total_games", std::to_string(all_games.size()).c_str());
    flateSetVar(flate, "db_last_update", ctime(&db_last_update));
    time_t next_update = db_last_update + std::atoi(config["update_db_timeout"].c_str());
    flateSetVar(flate, "db_next_update", ctime(&next_update));

    bgg_client::data::collection show_games = no_expansions;

    for (auto const & cgi : p_cgi) {
      if (cgi.first == "user") {
        show_games.clear();

        for (auto const & user : users_vector) {
          if (user.getBggNick() == cgi.second) {
            for (auto const & game : user.getCollection()) {
              if (not game.isExtension())
                show_games.push_back(game);
            }
          }
          else continue;
        }
      }
    }

    uint32_t games_index(0);
    for (auto const & g : show_games) {
      std::string string_owners;
      flateSetVar(flate, "game_name", g.getGameName().c_str());
      flateSetVar(flate, "game_description", g.getDescription().c_str());
      flateSetVar(flate, "game_thumbnail", g.getThumbnailUrl().c_str());
      std::string game_url = "http://boardgamegeek.com/boardgame/" + std::to_string(g.getGameId());
      flateSetVar(flate, "game_url", game_url.c_str());
      flateSetVar(flate, "game_anchor", std::to_string(g.getGameId()).c_str());
      flateSetVar(flate, "game_rank", g.getRank()? std::to_string(g.getRank()).c_str() : "not available");
      flateSetVar(flate, "game_published", g.getYearPublished()? std::to_string(g.getYearPublished()).c_str() : "too long ago");
      std::string game_authors;

      flateSetVar(flate, "games_javascript_name", g.getGameName().c_str());
      flateSetVar(flate, "games_javascript_anchor", std::to_string(g.getGameId()).c_str());
      if (++games_index == no_expansions.size())
        flateSetVar(flate, "games_javascript_comma_needed", " ");
      else
        flateSetVar(flate, "games_javascript_comma_needed", ",");
      flateDumpTableLine(flate, "games_javascript_variables");

      for (std::string const & author : g.getAuthors()) {
        game_authors += author + ", ";
      }
      game_authors = game_authors.substr(0, game_authors.find_last_of((',')));

      flateSetVar(flate, "game_authors", game_authors.c_str());
      flateSetVar(flate, "game_min_players", std::to_string(g.getMinPlayers()).c_str());
      flateSetVar(flate, "game_max_players", std::to_string(g.getMaxPlayers()).c_str());

      // Some games have either 0 or the same as the min player.
      flateSetVar(flate, "max_players_show", g.getMaxPlayers() != g.getMinPlayers() and g.getMaxPlayers() > 0? "inline" : "none");
      flateSetVar(flate, "game_playtime", std::to_string(g.getPlayingTime()).c_str());

      for (auto const & user : in_memory_db[g].owners) {
        std::string bgg_url = "http://boardgamegeek.com/user/" + user.getBggNick();
        string_owners += user.getForumNick() + " (" +
          "<a href=\"" + bgg_url + "\">" +
          decode(user.getBggNick()).c_str() + "</a>), ";
      }

      flateSetVar(flate, "has_wants", in_memory_db[g].wants_to_play.empty()? "none" : "inline");
      flateSetVar(flate, "wants_total", std::to_string(in_memory_db[g].wants_to_play.size()).c_str());
      std::string wants_list;

      for (auto const & user : in_memory_db[g].wants_to_play) {
        wants_list += user.getForumNick() + " ";
      }

      flateSetVar(flate, "wants_list", wants_list.c_str());

      // Remove last ","
      string_owners = string_owners.substr(0, string_owners.find_last_of(","));
      flateSetVar(flate, "game_owners", string_owners.c_str());

      // Get expansions
      flateSetVar(flate, "has_expansions", in_memory_db[g].expansions.empty() ? "none" : "inline");
      flateSetVar(flate, "expansions_total", std::to_string(in_memory_db[g].expansions.size()).c_str());

      for (auto const & exp : in_memory_db[g].expansions) {
        flateSetVar(flate, "expansion_name", exp.getGameName().c_str());
        flateSetVar(flate, "expansion_thumbnail", exp.getThumbnailUrl().c_str());
        flateSetVar(flate, "expansion_description", exp.getDescription().c_str());
        flateSetVar(flate, "expansion_id", std::to_string(exp.getGameId()).c_str());

        std::string exp_owners_string;

        for (auto const & user : in_memory_db[exp].owners) {
          std::string bgg_url = "http://boardgamegeek.com/user/" + user.getBggNick();
          exp_owners_string += user.getForumNick() + " (" +
            "<a href=\"" + bgg_url + "\">" +
            decode(user.getBggNick()).c_str() + "</a>), ";
        }

        // Remove last ","
        exp_owners_string = exp_owners_string.substr(0, exp_owners_string.find_last_of(","));
        flateSetVar(flate, "expansion_owners", exp_owners_string.c_str());

        flateDumpTableLine(flate, "game_expansions");
      }

      flateDumpTableLine(flate, "games_accordion");
    }

    synchro.unlock();

    char * buffer = flatePage(flate);
    ret = std::string(buffer);
    flateFreeMem(flate);

    delete buffer;
    return ret;
  };

  // Redirect servlet
  todo::web::servlet_t redirect = [&](std::string const & p_page, todo::url::cgi_t const & p_cgi, todo::http_request & p_request)->std::string {
    std::string ret;

    if (p_page.empty()) {
      p_request.m_code = todo::http_request::kPermanentRedirect;
      p_request["Location"] = "/games/";
      ret = "Please visit <a href=\"/games/\">the only servlet</a> of this website.";
    }
    else {
      p_request.m_code = todo::http_request::kTeaPot;
      ret = "I'm a teapot.";
    }

    return ret;
  };

  server.insert(servlet["address"], games_servlet);
  server.insert("/", redirect);
  server_ptr = &server;

  // Register signal handler as a lambda function.
  auto signal_handler = [](int signal)->void {
    std::cout << " -- Handler called, gracefully stopping server --\n";
    if (server_ptr != nullptr) server_ptr->stop();
  };

  signal(SIGINT, signal_handler);
  signal(SIGABRT, signal_handler);
  signal(SIGKILL, signal_handler);

  signal(SIGPIPE, [](int signal)->void {
    std::cerr << " --- Caught SIGPIPE, update failed, reopening connection\n";
    connection_ptr->close_connection();
    connection_ptr->open_connection();
  });

  signal(SIGUSR1, [](int signal)->void {
    std::cerr << " --- FORCING UPDATE OF THE BASE ---\n";
    force_base_update = true;
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
      if (++slept_time >= timeout or force_base_update) {
        update_db_function();
        force_base_update = false;
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
