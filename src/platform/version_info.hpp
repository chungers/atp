#ifndef ATP_PLATFORM_VERSION_INFO_H_
#define ATP_PLATFORM_VERSION_INFO_H_

#include <string>

using std::string;


namespace atp {
struct version_info
{
  const static string GIT_LAST_COMMIT_HASH;
  const static string GIT_LAST_COMMIT_MSG;
  const static string IB_API_VERSION;

  static void log(const string& label);

}; // version_info

} // atp


#endif //ATP_PLATFORM_VERSION_INFO_H_
