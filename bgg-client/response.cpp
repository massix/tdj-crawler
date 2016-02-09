//
//  response.cpp
//  tdjbgg
//
//  Created by Massimo Gengarelli on 08/02/16.
//  Copyright © 2016 Massimo Gengarelli. All rights reserved.
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
  }
  else {
    game.setExpands(0);
  }
}

void bgg_client::response::reset()
{
  m_valid = false;
  if (m_object != nullptr) delete m_object;

  m_rawJson.clear();
  m_games.clear();
}

bgg_client::response::~response()
{
  if (m_object != nullptr)
    delete m_object;
}