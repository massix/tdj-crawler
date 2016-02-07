#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <string>
#include "request.h"
#include "response.h"

namespace bgg_client
{

class connection
{
public:
    connection(std::string const & url);
    virtual ~connection();

    bool open_connection();

	// Synchronous connection
	bool send(bgg_client::request const & request,
			  bgg_client::response & response);

	// Asynchronous connection
	bool send(bgg_client::request const & request);
	bool recv(bgg_client::response & response);

private:
    std::string m_url;
    int m_socket;
	bgg_client::response * m_latest;
};

}

#endif // _CONNECTION_H_
