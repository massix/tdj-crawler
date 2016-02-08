#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <string>
#include <vector>
#include "request.h"
#include "response.h"

namespace bgg_client
{

class header_line
{
public:
  enum Type
  {
    HTTP_CODE,
    CONTENT_LENGTH,
    UNKNOWN
  };

  header_line(std::string const & header);
  virtual ~header_line();

  Type const & getType() const;

  uint32_t const asInt() const;
  std::string const & asString() const;

private:
  Type m_type;
  std::string m_value;

protected:
};

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
  std::string m_headersRaw;
  std::vector<header_line> m_parsedHeaders;

  bool relevant_information(std::string const & header, uint32_t & length, uint32_t & http_code);
};

}

#endif // _CONNECTION_H_
