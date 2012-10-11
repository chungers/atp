
#include <map>
#include <string>
#include <sstream>

#include <glog/logging.h>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

namespace ib {
namespace internal {

bool symbol_from_contract(const std::map<std::string, std::string>& nv,
                          std::string* output)
{
  // Build the symbol string here.
  try {
    std::ostringstream s;
    s << nv.at("symbol") << '.' << nv.at("secType");

    if (nv.at("secType") != "STK" && nv.at("secType") != "IND") {
      s << '.'
        << nv.at("expiry")
        << '.'
        << nv.at("strike")
        << '.'
        << nv.at("right");
    }
    *output = s.str();
    return true;

  } catch (...) {
    return false;
  }
};

} // namespace internal
} // namespace ib

