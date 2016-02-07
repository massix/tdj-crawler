//
//  main.cpp
//  tdjbgg
//
//  Created by Massimo Gengarelli on 07/02/16.
//  Copyright © 2016 Massimo Gengarelli. All rights reserved.
//

#include <iostream>
#include "connection.h"
#define BGG_URL "boardgamegeek"

int main(int argc, char *argv[])
{
	bgg_client::connection connection(BGG_URL);
	if (not connection.open_connection())
	{
		// Handle error
	}

	std::cout << "Hello world" << std::endl;
	return 0;
}