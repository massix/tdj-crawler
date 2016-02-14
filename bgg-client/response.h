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

#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include <string>
#include <json11.hpp>
#include "game.h"
#include "collection.h"

namespace bgg_client
{

class response
{
public:
  response();
  response(std::string const &json_raw);

  void initialize(std::string const &json_raw);

  bool is_valid() const { return m_valid; }
  data::collection & getGames()
  {
    return m_games;
  }

  data::collection & getWantsToPlay()
  {
    return m_wantsToPlay;
  }

  void reset();

  virtual ~response();

private:
  json11::Json * m_object;
  std::string m_rawJson;
  data::collection m_games;
  data::collection m_wantsToPlay;

  bool m_valid;

  void fill_game(json11::Json const & obj, bgg_client::data::game & game);
};

}

#endif //_RESPONSE_H_
