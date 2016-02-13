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

#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <string>
#include <vector>

namespace bgg_client
{

enum RequestType
{
  T_COLLECTION = 0,
  T_THING = 1
};

class request
{
public:
  virtual ~request();

  // Requests go here
  static request collection(std::string const & user);
  static request thing(uint32_t const & thing);
  std::string http_request() const;

private:
  request();
  RequestType m_type;
  std::vector<std::string> m_parameters;

protected:
};


}
#endif // _REQUEST_H_
