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

connection::connection(std::string const & url) : m_url(url), m_sleepTimeout(1)
{
}

connection::~connection()
{
}

bool connection::open_connection()
{
  syslog(LOG_INFO, "Opening connection");
  struct sockaddr_in l_serverAddress;
  struct hostent *l_server;
  int l_port(80);

  m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  openlog("tdj-crawler", LOG_PID | LOG_NOWAIT, LOG_USER);
  syslog(LOG_INFO, "Connection object created.");

  // Set keep-alive option
  int keep_alive = 1;
  if (setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive))) {
    perror("setsockopt(): ");
  }

  // Set reusable socket option
  int reusable = 1;
  if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &reusable, sizeof(keep_alive))) {
    perror("setsockopt(): ");
  }

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

bool connection::close_connection()
{
  return close(m_socket) < 0 ? false : true;
}

bool connection::send(const bgg_client::request &request, bgg_client::response &response)
{
  char buffer[4096];
  std::string full_response;
  ssize_t bytes;
  m_headersRaw.clear();
  m_parsedHeaders.clear();
  full_response.clear();

  bytes = ::send(m_socket,
                 request.http_request().c_str(),
                 request.http_request().length(),
                 0);

  if (bytes < 0)
    perror("write(): ");

  bzero(buffer, 4096);

  uint32_t l_contentLength(0);
  uint32_t l_httpCode(0);
  while (::recv(m_socket, buffer, 4096, 0)) {
    std::string full_line(buffer, 4096);

    if (relevant_information(full_line, l_contentLength, l_httpCode)) {
      full_line = full_line.substr(m_headersRaw.size() + 4, full_line.size());
    }

    full_response += (const char *) full_line.c_str();

    if (l_contentLength != 0 and full_response.size() >= l_contentLength)
      break;

    bzero(buffer, 4096);
  }

  if (l_httpCode == 500) {
    std::cerr << " --- 500, sleeping for " << m_sleepTimeout << " seconds and retrying.\n";
    sleep(m_sleepTimeout);
    m_sleepTimeout = std::min(m_sleepTimeout + m_sleepTimeout, 10);
    return send(request, response);
  }

  m_sleepTimeout = 1;
  full_response.resize(l_contentLength);
  response.initialize(full_response);

  return true;
}

bool connection::relevant_information(const std::string &header, uint32_t &length, uint32_t &http_code)
{
  std::string begin("HTTP/1.1");

  if (header.substr(0, begin.length()) == begin) {
    m_headersRaw = header.substr(0, header.find("\r\n\r\n"));

    ssize_t last_pos(0);
    ssize_t next_pos = m_headersRaw.find("\r\n");

    while (last_pos < next_pos) {
      std::string line(m_headersRaw.substr(last_pos, next_pos - last_pos));

      header_line header(line);
      if (header.getType() != header_line::UNKNOWN)
        m_parsedHeaders.push_back(header_line(line));

      last_pos = next_pos + 2;
      next_pos = m_headersRaw.substr(last_pos).find("\r\n") + last_pos;
    }

    for (header_line const & header : m_parsedHeaders) {
      if (header.getType() == header_line::CONTENT_LENGTH) {
        length = header.asInt();
      }

      if (header.getType() == header_line::HTTP_CODE) {
        http_code = header.asInt();
      }
    }

    return true;
  }

  return false;
}

header_line::header_line(std::string const &header)
{
  std::string content_length("Content-Length: ");
  std::string http_code("HTTP/1.1 ");

  if (header.substr(0, content_length.size()) == content_length) {
    m_type = CONTENT_LENGTH;
    m_value = header.substr(content_length.size(), header.find("\r\n"));
  }
  else if (header.substr(0, http_code.size()) == http_code) {
    m_type = HTTP_CODE;
    m_value = header.substr(http_code.size(), 3);
  }

  else m_type = UNKNOWN;
}

header_line::~header_line()
{
}

header_line::Type const & header_line::getType() const
{
  return m_type;
}

uint32_t const header_line::asInt() const
{
  return static_cast<uint32_t>(atoi(m_value.c_str()));
}

std::string const & header_line::asString() const
{
  return m_value;
}