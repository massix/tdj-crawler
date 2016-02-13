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

#ifndef database_hpp
#define database_hpp

#include <stdio.h>
#include <string>
#include <sqlite3.h>
#include <vector>
#include "user.h"
#include "game.h"
#include "collection.h"

namespace bgg_client { namespace data {

class database
{
public:
  database(std::string const & path);
  virtual ~database();

  /* Creates DB if it doesn't exist */
  void create_tables();

  /* Inserts or updates an user in the DB */
  void insert_update_user(user const & user);

  /* Inserts or updates a game in the DB */
  void insert_update_game(game const & game);

  /* Update the DB to reflect that an user owns a game.
   * Both things have to be in the DB already for this to work. 
   */
  void user_owns_game(user const& user, game const & game);

  /* Force an update of an user's collection */
  void update_user_collection(user const& user, collection const &collection);

  /* Modify the collection object to reflect the games owned by
   * the user in parameter
   */
  void collection_for_user(user const & user, collection & collection);

  /* Get a list of users that own a game */
  void users_for_game(std::vector<user> & list, game const & game);

  /* Returns true if the given name is in the DB already */
  bool game_exists(game const & game);

  /* Fills the game object for the game with the given id */
  void game_by_id(uint32_t const & id, game & game);

  /* Fills the game object for the game with the given name */
  void game_by_name(std::string const & name, game & game);

  /* Generic callback for the sqlite queries */
  int callback(int argc, char *argv[], char *columns[]);

  /* This should not be used elsewhere! Used to handle the callback */
  void set_last_query(std::string const & query) { m_lastQuery = query; };

  /* Get user by its bgg nickname */
  void user_by_bggnick(std::string const & nick, user & user);

  /* Get all the games in the collection */
  void all_games(collection & collection);

  /* Get only the games, not the expansions */
  void all_games_no_expansions(collection & collection);

  /* Get all the expansions for a given game */
  void expansions_for_game(game const & game, collection & expansions);

  /* Deletes a game from the DB */
  void delete_game(game const & game);

  /* Concurrency methods */
  void lock();
  void unlock();

private:
  sqlite3 * m_db;
  std::string m_dbLocation;
  std::string m_lastQuery;

  /* This is just a tempporary variable, do not rely on this one! */
  bool m_existsResult;

protected:
};

}}

#endif /* database_hpp */
