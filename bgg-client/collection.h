//
//  collection.hpp
//  tdjbgg
//
//  Created by Massimo Gengarelli on 08/02/16.
//  Copyright © 2016 Massimo Gengarelli. All rights reserved.
//

#ifndef collection_hpp
#define collection_hpp

#include <stdio.h>
#include <vector>
#include "game.h"

namespace bgg_client { namespace data {

class collection : public std::vector<game>
{
public:
  bool exists(uint32_t const &gameId) const;
  game getGameForId(uint32_t const & gameid) const;
};

}}

#endif /* collection_hpp */
