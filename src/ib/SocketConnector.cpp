
#include <sys/select.h>
#include <sys/time.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "log_levels.h"

#include "ib/AsioEClientSocket.hpp"
#include "ib/SocketConnector.hpp"

using namespace ib::internal;


namespace IBAPI {


SocketConnector::SocketConnector(int timeout)
    : timeout_(timeout)
{
  VLOG(VLOG_LEVEL_IBAPI_SOCKET_CONNECTOR) << "SocketConnector starting." << std::endl;
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
