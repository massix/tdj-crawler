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
