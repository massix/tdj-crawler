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
          g.setGameId(game["gameId"].int_value());
          g.setMinPlayers(game["minPlayers"].int_value());
          g.setMaxPlayers(game["maxPlayers"].int_value());
          g.setPlayingTime(game["playingTime"].int_value());
          g.setYearPublished(game["yearPublished"].int_value());
          g.setRank(game["rank"].int_value());
          g.setIsExtension(game["isExpansion"].bool_value());
          g.setGameName(game["name"].string_value());
          g.setThumbnailUrl(game["thumbnail"].string_value());
          m_games.push_back(g);
        }
      }
    }

    // This has to be a game then.
    else if (not ((*m_object)["gameId"].type() == json11::Json::NUL)) {
      bgg_client::data::game g;
      g.setGameId((*m_object)["gameId"].int_value());
      g.setMinPlayers((*m_object)["minPlayers"].int_value());
      g.setMaxPlayers((*m_object)["maxPlayers"].int_value());
      g.setPlayingTime((*m_object)["playingTime"].int_value());
      g.setYearPublished((*m_object)["yearPublished"].int_value());
      g.setRank((*m_object)["rank"].int_value());
      g.setIsExtension((*m_object)["isExpansion"].bool_value());
      g.setGameName((*m_object)["name"].string_value());
      g.setThumbnailUrl((*m_object)["thumbnail"].string_value());
      g.setDescription((*m_object)["description"].string_value());
      m_games.push_back(g);
    }
  }

  m_valid = error.empty();
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