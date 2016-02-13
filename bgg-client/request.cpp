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

#include "request.h"

#define COLLECTION_URL "/collection/"
#define THING_URL "/thing/"
#define HOST "bgg-json.azurewebsites.net"

bgg_client::request::request()
{

}

bgg_client::request::~request()
{

}

bgg_client::request bgg_client::request::collection(const std::string &user)
{
  bgg_client::request r;

  r.m_type = bgg_client::T_COLLECTION;
  r.m_parameters.push_back(user);

  return r;
}

bgg_client::request bgg_client::request::thing(const uint32_t &thing)
{
  bgg_client::request r;

  r.m_type = bgg_client::T_THING;
  r.m_parameters.push_back(std::to_string(thing));

  return r;
}

std::string bgg_client::request::http_request() const
{
  std::string request;

  switch (m_type) {
    case T_COLLECTION:
      request = "GET " + std::string(COLLECTION_URL) + m_parameters[0] + " HTTP/1.1\r\n";
      break;
    case T_THING:
      request = "GET " + std::string(THING_URL) + m_parameters[0] + " HTTP/1.1\r\n";
      break;
    default:
      break;
  }

  request += "Host: " + std::string(HOST) + "\r\n";
  request += "User-Agent: tdj-crawler/dev\r\n";
  request += "Connection: keep-alive\r\n";
  request += "\r\n";

  return request;
}