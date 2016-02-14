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

#include "response.h"
#include "game.h"
#include <iostream>

bgg_client::response::response() : m_object(nullptr), m_valid(false)
{
}

bgg_client::response::response(std::string const &json_raw) :
  m_object(nullptr), m_valid(false)
{
  initialize(json_raw);
}

void bgg_client::response::initialize(const std::string &json_raw)
{
  std::string error;
  m_object = new json11::Json(json11::Json::parse(json_raw, error));
  m_rawJson = json_raw;

  if (not error.empty()) {
    std::cerr << error << std::endl;
  }

  // Parse the json response
  else {
    
    // This is a collection, each object represents a game
    if (m_object->is_array()) {
      for (auto &game : m_object->array_items()) {
        if (game["owned"].bool_value()) {
          bgg_client::data::game g;
          fill_game(game, g);
          m_games.push_back(g);
        }
        else if (game["wantToPlay"].bool_value()) {
          bgg_client::data::game g;
          fill_game(game, g);
          m_wantsToPlay.push_back(g);
        }
      }
    }

    // This has to be a game then.
    else if (not ((*m_object)["gameId"].type() == json11::Json::NUL)) {
      bgg_client::data::game g;
      fill_game(*m_object, g);
      m_games.push_back(g);
    }
  }

  m_valid = error.empty();
}

void bgg_client::response::fill_game(const json11::Json &obj, bgg_client::data::game &game)
{
  game.setGameId(obj["gameId"].int_value());
  game.setMinPlayers(obj["minPlayers"].int_value());
  game.setMaxPlayers(obj["maxPlayers"].int_value());
  game.setPlayingTime(obj["playingTime"].int_value());
  game.setYearPublished(obj["yearPublished"].int_value());
  game.setRank(obj["rank"].int_value());
  game.setIsExtension(obj["isExpansion"].bool_value());
  game.setGameName(obj["name"].string_value());
  game.setThumbnailUrl(obj["thumbnail"].string_value());
  game.setDescription(obj["description"].string_value());

  if (game.isExtension() and not obj["expands"].array_items().empty()) {
    game.setExpands(obj["expands"].array_items()[0]["gameId"].int_value());
    game.setIsExtension(true);
  }
  else if (not obj["expands"].array_items().empty()) {
    game.setExpands(obj["expands"].array_items()[0]["gameId"].int_value());
    game.setIsExtension(true);
  }
  else {
    game.setExpands(0);
    game.setIsExtension(false);
  }

  if (not obj["designers"].array_items().empty()) {
    for (auto const & designer : obj["designers"].array_items()) {
      game.setAuthor(designer.string_value());
    }
  }
}

void bgg_client::response::reset()
{
  m_valid = false;
  if (m_object != nullptr) delete m_object;

  m_rawJson.clear();
  m_games.clear();
  m_wantsToPlay.clear();
}

bgg_client::response::~response()
{
  if (m_object != nullptr)
    delete m_object;
}