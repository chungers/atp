
#include <sys/select.h>
#include <sys/time.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "ib/AsioEClientSocket.hpp"
#include "ib/SocketConnector.hpp"

using namespace ib::internal;
using namespace std;

namespace IBAPI {


const int VLOG_LEVEL = 2;


SocketConnector::SocketConnector(int timeout)
    : timeout_(timeout)
{
}

SocketConnector::~SocketConnector()
{
}


/**
 * QuickFIX implementation returns the socket fd.  In our case, we simply
 * return the clientId.
 */
int SocketConnector::connect(const string& host,
                             unsigned int port,
                             unsigned int clientId,
                             Strategy* strategy)
{
  return clientId;
}


} // namespace IBAPI
