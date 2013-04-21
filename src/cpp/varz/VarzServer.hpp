#ifndef ATP_VARZ_SERVER_H_
#define ATP_VARZ_SERVER_H_

#include <boost/scoped_ptr.hpp>

namespace atp {
namespace varz {

/// Http server for VARZ monitoring
class VarzServer
{
 public:
  VarzServer(int port, int num_threads = 2);
  ~VarzServer();

  /// Starts the varz server
  void start();

  /// Stops the varz server
  void stop();

 private:
  class implementation;
  boost::scoped_ptr<implementation> impl_;
};

} // namespace varz
} // namespace atp

#endif //ATP_VARZ_SERVER_H_
