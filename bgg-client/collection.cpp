//
//  collection.cpp
//  tdjbgg
//
//  Created by Massimo Gengarelli on 08/02/16.
//  Copyright © 2016 Massimo Gengarelli. All rights reserved.
//

#include "collection.h"
#include "game.h"

bool bgg_client::data::collection::exists(const uint32_t &gameId) const {

  for (auto const & game : *this) {
    if (game.getGameId() == gameId)
      return true;
  }

  return false;
}

bgg_client::data::game bgg_client::data::collection::getGameForId(const uint32_t &gameid) const {

  for (auto const & game : *this) {
    if (game.getGameId() == gameid) {
      return game;
    }
  }

  return bgg_client::data::game();
}