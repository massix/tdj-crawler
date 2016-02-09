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
#include "connection.h"
#include "request.h"
#include "response.h"
#include "collection.h"
#include "user.h"
#include "game.h"
#include "database.h"

#define BGG_URL "bgg-json.azurewebsites.net"

int main(int argc, char *argv[])
{
	bgg_client::connection connection(BGG_URL);
  bgg_client::data::database db("./test.db");
  db.create_tables();

  if (not connection.open_connection()) {
    std::cerr << "Not connected" << std::endl;
	}

  std::vector<bgg_client::data::user> l_users;
  l_users.push_back(bgg_client::data::user("massi_x", "massi_x"));
  l_users.push_back(bgg_client::data::user("beyondmaster", "Calimero"));
  l_users.push_back(bgg_client::data::user("Chakado", "Chakado"));
  l_users.push_back(bgg_client::data::user("Pyvert", "Pyvert"));
  l_users.push_back(bgg_client::data::user("Platypus_Lord", "Flo"));

  bgg_client::response response;

  for (auto & user : l_users) {
    response.reset();
    connection.send(bgg_client::request::collection(user.getBggNick()), response);
    db.insert_update_user(user);

    if (response.is_valid()) {
      for (auto & game : response.getGames()) {
        db.user_owns_game(user, game);

        if (not db.game_exists(game)) {

          // Query BGG to get more details about the game itself
          bgg_client::response r;
          connection.send(bgg_client::request::thing(game.getGameId()), r);

          if (r.is_valid()) {
            std::cout << " => Adding '" << r.getGames()[0].getGameName() << "' to the DB\n";
            db.insert_update_game(r.getGames()[0]);
          }
        }
      }
    }
  }

  bgg_client::data::collection all_games;
  bgg_client::data::collection no_expansions;
  db.all_games(all_games);
  db.all_games_no_expansions(no_expansions);

  std::cout << all_games.size() << " total games in DB (expansions included)\n";
  std::cout << no_expansions.size() << " total games in DB (no expansions)\n";

  bgg_client::data::game w;
  bgg_client::data::collection exp;
  db.game_by_name("7 Wonders", w);
  db.expansions_for_game(w, exp);

  std::cout << "Expansions for game " << w.getGameName() << "\n";
  for (auto & expansion : exp) {
    std::cout << " - " << expansion.getGameName() << "\n";
  }

  todo::config config("./conf/tdj-crawler.conf");
  if (config.parse_config()) {
    std::cout << "Ready to go!\n";
  }
  
  todo::web server(&config);

  // There should be only a single servlet
  auto const & servlet = config.getServlets().front();

//  std::cout << "servlet resources and templates: " << servlet["resources"] << " " << servlet["templates"] << "\n";

  // Register the main servlet.
  todo::web::servlet_t games_servlet = [&](std::string const & p_page, todo::url::cgi_t const & p_cgi, todo::http_request & p_request)->std::string {
    std::string ret;
    std::string content_type = "text/html";
    p_request.m_code = todo::http_request::kOkay;

    Flate * flate = nullptr;
    flateSetFile(&flate, std::string(servlet["templates"] + "games_template.html").c_str());

    std::vector<std::string> random_greeters =
    {
      "All games owned.",
      "Because games matter.",
      "Simply the best!",
      "We won't bite you.",
      "We accept Italians too!"
    };

    std::srand((uint32_t) time(0));

    for (auto const & g : no_expansions) {
      std::string string_owners;
      flateSetVar(flate, "game_name", g.getGameName().c_str());
      flateSetVar(flate, "game_description", g.getDescription().c_str());
      flateSetVar(flate, "game_thumbnail", g.getThumbnailUrl().c_str());
      std::string game_url = "http://boardgamegeek.com/boardgame/" + std::to_string(g.getGameId());
      flateSetVar(flate, "game_url", game_url.c_str());
      flateSetVar(flate, "random_greet", random_greeters[(std::rand() % random_greeters.size())].c_str());

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
        exp_owners_string = string_owners.substr(0, string_owners.find_last_of(","));
        flateSetVar(flate, "expansion_owners", exp_owners_string.c_str());

        flateDumpTableLine(flate, "game_expansions");
      }

      flateDumpTableLine(flate, "games_accordion");
    }

    ret = flatePage(flate);
    return ret;
  };

  server.insert(servlet["address"], games_servlet);
  server.run();

  return 0;
}