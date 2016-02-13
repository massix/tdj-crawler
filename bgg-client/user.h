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

#ifndef user_hpp
#define user_hpp

#include <stdio.h>
#include <string>
#include "collection.h"

namespace bgg_client { namespace data {

class user
{
public:
  user() {};
  user(std::string const& bgg_nick, std::string const& forum_nick);
  virtual ~user() {};

  std::string const & getBggNick() const { return m_bggNick; }
  std::string const & getForumNick() const { return m_forumNick; }
  void setBggNick(std::string const & nick) { m_bggNick = nick; }
  void setForumNick(std::string const & nick) { m_forumNick = nick; }

  void setCollection(collection const& collection) { m_collection = collection; }
  collection const & getCollection() const { return m_collection; }
  collection & accessCollection() { return m_collection; }

private:
  std::string m_bggNick;
  std::string m_forumNick;

  collection m_collection;

protected:
};

}}

#endif /* user_hpp */
