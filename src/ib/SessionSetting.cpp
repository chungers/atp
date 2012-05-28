
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>

#include "ib/SessionSetting.hpp"


namespace IBAPI {


using namespace std;

bool SessionSetting::ParseSessionSettingFromString(
    const string& input,
    unsigned int* sessionId,
    string* host,
    unsigned int* port,
    string* reactor)
{
  /// format:  {session_id}={gateway_ip_port}@{reactor_endpoint}
  stringstream iss(input);

  string session_id;
  std::getline(iss, session_id, '=');
  *sessionId = boost::lexical_cast<unsigned int>(session_id);

  string gateway_ip_port;
  std::getline(iss, gateway_ip_port, '@');

  vector<string> gatewayParts;
  boost::split(gatewayParts, gateway_ip_port, boost::is_any_of(":"));

  host->assign(gatewayParts[0]);
  *port = boost::lexical_cast<int>(gatewayParts[1]);

  string reactor_endpoint;
  iss >> reactor_endpoint;
  reactor->assign(reactor_endpoint);

  return true;
}

} // IBAPI

