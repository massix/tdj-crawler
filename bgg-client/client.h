#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "connection.h"

namespace bgg_client
{

class client
{
public:
    client();
    virtual ~client();

private:
    connection * m_connection;

};

}

#endif _CLIENT_H_
