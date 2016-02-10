//
//  database.cpp
//  tdjbgg
//
//  Created by Massimo Gengarelli on 08/02/16.
//  Copyright © 2016 Massimo Gengarelli. All rights reserved.
//

#include "database.h"
#include <iostream>

/*
 *    ____      _ _ _                _
 *   / ___|__ _| | | |__   __ _  ___| | _____
 *  | |   / _` | | | '_ \ / _` |/ __| |/ / __|
 *  | |__| (_| | | | |_) | (_| | (__|   <\__ \
 *   \____\__,_|_|_|_.__/ \__,_|\___|_|\_\___/
 *
 */

static int users_for_game_callback(void *data, int argc, char *argv[], char *column[])
{
  std::vector<bgg_client::data::user> * list = (std::vector<bgg_client::data::user>*) data;
  bgg_client::data::user user;

  if (argv[0] != nullptr) {
    user.setBggNick(argv[0]);
    list->push_back(user);
  }

  return 0;
}

// Generic callback to check if a field exists.
static int exists_callback(void *data, int argc, char *argv[], char *colName[])
{
  bgg_client::data::database * object = reinterpret_cast<bgg_client::data::database*>(data);
  object->set_last_query("exists");
  return object->callback(argc, argv, colName);
}

static int select_user_callback(void *data, int argc, char *argv[], char *columns[])
{
  bgg_client::data::user * user = (bgg_client::data::user *) data;
  user->setForumNick("");
  user->setBggNick("");
  user->accessCollection().clear();

  for (uint32_t i = 0; i < argc; i++) {
    if (argv[i] == nullptr) break;

    if (std::string(columns[i]) == "bggnick") {
      user->setBggNick(std::string(argv[i]));
    }

    if (std::string(columns[i]) == "forumnick") {
      user->setForumNick(std::string(argv[i]));
    }

    if (std::string(columns[i]) == "games") {
      std::string plain_list(argv[i]);
      ssize_t last_pos(0);
      ssize_t pos(0);

      while ((pos = plain_list.find(" ", last_pos)) != std::string::npos) {
        bgg_client::data::game g;
        g.setGameId(std::atoi(plain_list.substr(last_pos, pos).c_str()));
        user->accessCollection().push_back(g);
        last_pos = pos + 1;
      }
    }
  }

  return 0;
}

static int game_by_id_callback(void *data, int argc, char *argv[], char *column[])
{
  bgg_client::data::game * game = (bgg_client::data::game *) data;

  for (uint32_t i = 0; i < argc; i++) {
    if (argv[i] == nullptr) break;
    std::string col(column[i]);

    if (col == "id")
      game->setGameId(atoi(argv[i]));

    else if (col == "name")
      game->setGameName(argv[i]);

    else if (col == "description")
      game->setDescription(argv[i]);

    else if (col == "minplayers")
      game->setMinPlayers(atoi(argv[i]));

    else if (col == "maxplayers")
      game->setMaxPlayers(atoi(argv[i]));

    else if (col == "playingtime")
      game->setPlayingTime(atoi(argv[i]));

    else if (col == "yearpublished")
      game->setYearPublished(atoi(argv[i]));

    else if (col == "rank")
      game->setRank(atoi(argv[i]));

    else if (col == "extension")
      game->setIsExtension(atoi(argv[i]));

    else if (col == "thumbnail")
      game->setThumbnailUrl(argv[i]);

    else if (col == "expands")
      game->setExpands(atoi(argv[i]));

    else if (col == "authors") {
      std::string authors(argv[i]);
      ssize_t last_pos(0);
      ssize_t pos(0);

      while ((pos = authors.find(", ", last_pos)) != std::string::npos) {
        game->setAuthor(authors.substr(last_pos, pos).c_str());
        last_pos = pos + 1;
      }

      std::string last_author = authors.substr(last_pos);
      if (not last_author.empty()) {
        game->setAuthor(last_author);
      }
    }
  }
  return 0;
}

static int get_all_games_callback(void *data, int argc ,char *argv[], char *column[])
{
  bgg_client::data::collection * collection = (bgg_client::data::collection *) data;
  collection->push_back(bgg_client::data::game(atoi(argv[0])));

  return 0;
}


/*
 *    __  __      _   _               _
 *   |  \/  | ___| |_| |__   ___   __| |___
 *   | |\/| |/ _ \ __| '_ \ / _ \ / _` / __|
 *   | |  | |  __/ |_| | | | (_) | (_| \__ \
 *   |_|  |_|\___|\__|_| |_|\___/ \__,_|___/
 */


bgg_client::data::database::database(std::string const & path)
  : m_db(nullptr), m_dbLocation(path)
{
  sqlite3_open(m_dbLocation.c_str(), &m_db);
}

bgg_client::data::database::~database()
{
  if (m_db != nullptr) {
    sqlite3_close(m_db);
  }
}

void bgg_client::data::database::create_tables()
{
  std::string games_table_query, users_table_query;

  games_table_query =
    "create table if not exists games("
    "id int primary_key not null, "
    "name text not null, "
    "description text, "
    "minplayers int, "
    "maxplayers int, "
    "playingtime int, "
    "yearpublished int, "
    "rank int, "
    "extension bool, "
    "thumbnail text, "
    "authors text, "
    "expands int"
    ");";

  users_table_query =
    "create table if not exists users("
    "bggnick text primary_key not null, "
    "forumnick text primary_key not null, "
    "games text" // space-separated list of games id
    ");";

  sqlite3_exec(m_db, games_table_query.c_str(), 0, 0, 0);
  sqlite3_exec(m_db, users_table_query.c_str(), 0, 0, 0);
}

void bgg_client::data::database::insert_update_user(const bgg_client::data::user &user)
{
  std::string insert_user_query;
  std::string imploded_games;

  // Implode all games into a single string
  for (auto const & g : user.getCollection()) {
    imploded_games += std::to_string(g.getGameId()) + " ";
  }

  insert_user_query =
    "insert into users(bggnick, forumnick, games) select '" + user.getBggNick() + "', "
    "'" + user.getForumNick() + "', "
    "'" + imploded_games + "'"
    "where not exists(select 1 from users where bggnick = '" + user.getBggNick() + "')"
    ";";

  sqlite3_exec(m_db, insert_user_query.c_str(), 0, 0, 0);
}

void bgg_client::data::database::insert_update_game(const bgg_client::data::game &game)
{
  std::string insert_game_query;
  int rc(0);

  std::string escapedDescription = game.getDescription();
  ssize_t pos(0);
  while ((pos = escapedDescription.find("\"", pos)) != std::string::npos) {
    escapedDescription.replace(pos, 1, "&quot;");
    pos += 6;
  }

  std::string escapedName = game.getGameName();
  pos = 0;
  while ((pos = escapedName.find("\"", pos)) != std::string::npos) {
    escapedName.replace(pos, 1, "\\\"");
    pos += 1;
  }

  std::string authors;
  for (auto const & author : game.getAuthors()) {
    authors += author + ", ";
  }

  authors = authors.substr(0, authors.find_last_of(","));

  insert_game_query =
    "insert into games("
    "id, name, description, minplayers, maxplayers, playingtime, "
    "yearpublished, rank, extension, thumbnail, authors, expands) "
    "select "
    + std::to_string(game.getGameId()) + ", "
    "\"" + escapedName + "\", "
    "\"" + escapedDescription + "\", "
    + std::to_string(game.getMinPlayers()) + ", "
    + std::to_string(game.getMaxPlayers()) + ", "
    + std::to_string(game.getPlayingTime()) + ", "
    + std::to_string(game.getYearPublished()) + ", "
    + std::to_string(game.getRank()) + ", "
    + std::to_string(game.isExtension()) + ", "
    "'" + game.getThumbnailUrl() + "', "
    "'" + authors + "', "
    + std::to_string(game.getExpands()) + " "
    "where not exists("
    "select 1 from games where id = " + std::to_string(game.getGameId()) + ")"
    ";";

  sqlite3_exec(m_db, insert_game_query.c_str(), 0, 0, 0);
  if (rc != SQLITE_OK) {
    std::cerr << "Could not add " << game.getGameName() << "\n";
  }
}

bool bgg_client::data::database::game_exists(const bgg_client::data::game &game)
{
  std::string check_if_exists_query;
  int rc(0);

  check_if_exists_query =
    "select exists(select 1 from games where id = "
    + std::to_string(game.getGameId()) + ");";

  rc = sqlite3_exec(m_db,
                    check_if_exists_query.c_str(),
                    exists_callback,
                    this,
                    0);

  return m_existsResult;
}

void bgg_client::data::database::user_owns_game(bgg_client::data::user const &user, const bgg_client::data::game &game)
{
  // Fetch latest information
  bgg_client::data::user result;
  user_by_bggnick(user.getBggNick(), result);

  bool found(false);

  for (auto const & g : result.getCollection()) {
    if (g.getGameId() == game.getGameId()) {
      found = true;
      break;
    }
  }

  if (not found) {
    result.accessCollection().push_back(game);
    update_user_collection(result, result.getCollection());
  }
}

void bgg_client::data::database::update_user_collection(const bgg_client::data::user &user, const bgg_client::data::collection &collection)
{
  std::string imploded_games;
  std::string update_query;

  // Implode all games into a single string
  for (auto const & g : user.getCollection()) {
    imploded_games += std::to_string(g.getGameId()) + " ";
  }

  update_query = "update users set games = '" + imploded_games + "' "
                 "where bggnick = '" + user.getBggNick() + "';";

  sqlite3_exec(m_db, update_query.c_str(), 0, 0, 0);
}

void bgg_client::data::database::user_by_bggnick(const std::string &nick, bgg_client::data::user &user)
{
  std::string get_user_query;
  get_user_query = "select * from users where bggnick = \"" + nick + "\";";

  sqlite3_exec(m_db, get_user_query.c_str(), select_user_callback, &user, 0);
}

void bgg_client::data::database::users_for_game(std::vector<bgg_client::data::user> &list, const bgg_client::data::game &game)
{
  std::string get_users_for_game_query;
  get_users_for_game_query = "select bggnick from users where ' ' || games || ' ' like '% " + std::to_string(game.getGameId())+ " %';";
  list.clear();

  sqlite3_exec(m_db, get_users_for_game_query.c_str(), users_for_game_callback, &list, 0);

  for (auto & user : list) {
    user_by_bggnick(user.getBggNick(), user);
  }
}

int bgg_client::data::database::callback(int argc, char **argv, char **columns)
{
  if (m_lastQuery == "exists") {
    m_existsResult = atoi(argv[0]);
  }

  return 0;
}

void bgg_client::data::database::game_by_id(const uint32_t &id, bgg_client::data::game &game)
{
  std::string get_game_query;
  get_game_query = "select * from games where id = " + std::to_string(id) + ";";

  sqlite3_exec(m_db, get_game_query.c_str(), game_by_id_callback, &game, 0);
}

void bgg_client::data::database::game_by_name(const std::string &name, bgg_client::data::game &game)
{
  std::string get_game_query;
  get_game_query = "select * from games where name = '" + name + "' limit 1;";

  sqlite3_exec(m_db, get_game_query.c_str(), game_by_id_callback, &game, 0);
}

void bgg_client::data::database::all_games(bgg_client::data::collection &collection)
{
  std::string get_all_games;
  get_all_games = "select id from games order by name asc;";

  sqlite3_exec(m_db, get_all_games.c_str(), get_all_games_callback, &collection, 0);

  for (auto& g : collection) {
    game_by_id(g.getGameId(), g);
  }
}

void bgg_client::data::database::all_games_no_expansions(bgg_client::data::collection &collection)
{
  std::string get_all_games_query;
  get_all_games_query = "select id from games where expands = 0 order by name asc";

  sqlite3_exec(m_db, get_all_games_query.c_str(), get_all_games_callback, &collection, 0);

  for (auto& g: collection) {
    game_by_id(g.getGameId(), g);
  }
}

void bgg_client::data::database::expansions_for_game(const bgg_client::data::game &game, bgg_client::data::collection &expansions)
{
  std::string get_all_expansions_query;
  get_all_expansions_query = "select id from games where expands = " + std::to_string(game.getGameId()) + ";";

  sqlite3_exec(m_db, get_all_expansions_query.c_str(), get_all_games_callback, &expansions, 0);

  for (auto& g: expansions) {
    game_by_id(g.getGameId(), g);
  }
}
