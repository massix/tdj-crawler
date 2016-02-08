//
//  user.hpp
//  tdjbgg
//
//  Created by Massimo Gengarelli on 08/02/16.
//  Copyright © 2016 Massimo Gengarelli. All rights reserved.
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
