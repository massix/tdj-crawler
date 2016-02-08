#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include <string>
#include <json11.hpp>
#include "game.h"

namespace bgg_client
{

class response
{
public:
  response();
  response(std::string const &json_raw);

  void initialize(std::string const &json_raw);

  bool is_valid() const { return m_valid; }
  std::vector<bgg_client::data::game> & getGames()
  {
    return m_games;
  }

  void reset();

  virtual ~response();

private:
  json11::Json * m_object;
  std::string m_rawJson;
  std::vector<::bgg_client::data::game> m_games;

  bool m_valid;
};

}

#endif //_RESPONSE_H_
