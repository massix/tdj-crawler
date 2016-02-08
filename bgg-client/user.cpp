//
//  user.cpp
//  tdjbgg
//
//  Created by Massimo Gengarelli on 08/02/16.
//  Copyright © 2016 Massimo Gengarelli. All rights reserved.
//

#include "user.h"

bgg_client::data::user::user(std::string const &bgg_nick, std::string const &forum_nick)
: m_bggNick(bgg_nick), m_forumNick(forum_nick)
{
}