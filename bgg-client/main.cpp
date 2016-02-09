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

  server.run();

  return 0;
}