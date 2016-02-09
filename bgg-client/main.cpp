//
//  main.cpp
//  tdjbgg
//
//  Created by Massimo Gengarelli on 07/02/16.
//  Copyright © 2016 Massimo Gengarelli. All rights reserved.
//

#include <iostream>
#include <vector>
#include "connection.h"
#include "request.h"
#include "response.h"
#include "collection.h"
#include "user.h"
#include "game.h"
#include "database.h"
#include <sqlite3.h>
#include <web.h>
#include <config.h>
#include <flate.h>

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
  l_users.push_back(bgg_client::data::user("dizzark", "DanieleTascini"));

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

    for (auto const & g : no_expansions) {
      std::string string_owners;
      flateSetVar(flate, "game_name", g.getGameName().c_str());
      flateSetVar(flate, "game_description", g.getDescription().c_str());
      flateSetVar(flate, "game_thumbnail", g.getThumbnailUrl().c_str());

      std::string game_url = "http://boardgamegeek.com/boardgame/" + std::to_string(g.getGameId());
      flateSetVar(flate, "game_url", game_url.c_str());

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
      flateDumpTableLine(flate, "games_accordion");
    }

    ret = flatePage(flate);
    return ret;
  };

  server.insert(servlet["address"], games_servlet);

//  // Register a dump configuration servlet
//  m_servlets["/debug/"] = [&](std::string const & p_page, url::cgi_t const & p_cgi, http_request & p_request)->std::string {
//    std::string l_ret;
//    std::string l_contentType = "text/html";
//    p_request.m_code = http_request::kOkay;
//
//    if (p_page.empty()) {
//      Flate * l_flate = NULL;
//      flateSetFile(&l_flate, std::string(m_templates + "debug_template.html").c_str());
//
//      for (config::value_type const & c_key : (*m_config)) {
//        flateSetVar(l_flate, "key", c_key.first.c_str());
//        flateSetVar(l_flate, "value", c_key.second.c_str());
//        flateDumpTableLine(l_flate, "config");
//      }
//
//      for (url::cgi_t::value_type const & l_value : p_cgi) {
//        if (l_value.first != "submit") {
//          flateSetVar(l_flate, "cgi_key", l_value.first.c_str());
//          flateSetVar(l_flate, "cgi_value", l_value.second.c_str());
//          flateDumpTableLine(l_flate, "cgi");
//        }
//      }
//
//      for (servlet const & l_servlet : m_config->getServlets()) {
//        flateSetVar(l_flate, "servlet_name", l_servlet["name"].c_str());
//
//        for (servlet::value_type const & l_value : l_servlet) {
//          flateSetVar(l_flate, "servlet_key", l_value.first.c_str());
//          flateSetVar(l_flate, "servlet_value", l_value.second.c_str());
//          flateDumpTableLine(l_flate, "servlet_config");
//        }
//
//        flateDumpTableLine(l_flate, "servlet_container");
//      }
//
//      l_ret = flatePage(l_flate);
//    }
//    else if (not get_content_of_file(m_templates + p_page, l_ret, l_contentType)) {
//      p_request.m_code = http_request::kNotFound;
//      l_ret = "Page not found";
//    }
//
//    p_request["Content-Type"] = l_contentType;
//    return l_ret;
//  };


  server.run();

  return 0;
}