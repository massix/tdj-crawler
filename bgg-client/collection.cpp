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