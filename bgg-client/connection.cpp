#include "connection.h"

#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include <iostream>
#include <fstream>
#include <unistd.h>

using namespace bgg_client;

connection::connection(std::string const & url) : m_url(url)
{
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    openlog("tdj-crawler", LOG_PID | LOG_NOWAIT, LOG_USER);
    syslog(LOG_INFO, "Connection object created.");
}

connection::~connection()
{
    closelog();
}

bool connection::open_connection()
{
    syslog(LOG_INFO, "Opening connection");
	struct sockaddr_in l_serverAddress;
	struct hostent *l_server;
	int l_port(80);

	l_server = gethostbyname(m_url.c_str());

	bzero(&l_serverAddress, sizeof(l_serverAddress));

	l_serverAddress.sin_family = AF_INET;
	bcopy((char *) l_server->h_addr_list[0],
		  (char *) &l_serverAddress.sin_addr.s_addr,
		  l_server->h_length);
	l_serverAddress.sin_port = htons(l_port);

	return connect(m_socket,
				   (struct sockaddr *) &l_serverAddress,
				   sizeof(l_serverAddress)) < 0 ? false : true;
}
