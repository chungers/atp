
#include <map>
#include <list>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <Shared/EWrapper.h>

#include "common.hpp"
#include "log_levels.h"
#include "ib/SocketInitiator.hpp"
#include "ib/EWrapperFactory.hpp"
#include "ib/SessionID.hpp"
#include "zmq/Publisher.hpp"


using ib::internal::EWrapperFactory;

namespace ib {
namespace internal {

static bool isInProc(const std::string& address)
{
  return boost::starts_with(address, "inproc://");
}

} // namespace internal
} // namespace ib
namespace IBAPI {

class SocketInitiatorImpl : public SocketInitiator {

 public :
  SocketInitiatorImpl(Application& app,
                      std::list<SessionSetting>& settings,
                      SocketConnector::Strategy& strategy) :
      application_(app),
      sessionSettings_(settings),
      strategy_(strategy)
  { }

  ~SocketInitiatorImpl()
  {
    std::map< int, ChannelPublisher >::iterator publisher =
        channelPublishers_.begin();
    for (; publisher != channelPublishers_.end(); ++publisher) {
      if (publisher->second) {
        IBAPI_SOCKET_INITIATOR_LOGGER
            << "Destroying publisher for channel " << publisher->first;
        delete publisher->second;
      }
    }
  }

  /// Starts publisher at the given zmq address.
  void startPublisher(int channel, const std::string& address)
      throw ( ConfigError, RuntimeError )
  {
    boost::unique_lock<boost::mutex> lock(mutex_);

    if (outboundContextPtr_ == NULL) {
      outboundContextPtr_ = new zmq::context_t(1);
    }

    // If we are running the publisher, then use in process context
    // for the connector (outbound) and publisher.
    const std::string& connectorOutboundAddress = "inproc://connectors";
    if (outboundChannels_.find(channel) == outboundChannels_.end()) {
      // start publisher
      IBAPI_SOCKET_INITIATOR_LOGGER
          << "Starting embedded publisher for channel " << channel
          << " at " << address;
      outboundChannels_[channel] = connectorOutboundAddress;

      channelPublishers_[channel] = new atp::zmq::Publisher(
          connectorOutboundAddress,
          address, outboundContextPtr_);

    } else {
      IBAPI_SOCKET_INITIATOR_LOGGER
          << "Publisher for channel " << channel << " already running: "
          << outboundChannels_[channel];
    }

  }


  /// @overload Initiator
  void start() throw ( ConfigError, RuntimeError )
  {
    boost::unique_lock<boost::mutex> lock(mutex_);

    if (outboundContextPtr_ == NULL) {
      outboundContextPtr_ = new zmq::context_t(1);
    }

    // Start up the socket connectors one by one.
    std::list<SessionSetting>::iterator itr;
    for (itr = sessionSettings_.begin();
         itr != sessionSettings_.end();
         ++itr) {

      IBAPI_SOCKET_INITIATOR_LOGGER
          << "Starting connector "
          << itr->getConnectionId()
          << ", inbound: " << itr->getConnectorReactorAddress()
          << ", context: " << outboundContextPtr_;

      SessionID sessionId = static_cast<SessionID>(itr->getConnectionId());
      boost::shared_ptr<SocketConnector> s =
          boost::shared_ptr<SocketConnector>(
              new SocketConnector(itr->getConnectorReactorAddress(),
                                  outboundChannels_,
                                  application_, sessionId,
                                  inboundContextPtr_,
                                  outboundContextPtr_));

      // Register this in a map for clean up later.
      socketConnectors_[sessionId] = s;

      IBAPI_SOCKET_INITIATOR_LOGGER << "SocketConnector: " << *itr;
      s->connect(itr->getIp(), itr->getPort(), sessionId, &strategy_);
    }
  }

  /// @overload Initiator
  void block() throw ( ConfigError, RuntimeError )
  {

  }

  /// @overload Initiator
  void stop(double timeout)
  {
    IBAPI_SOCKET_INITIATOR_LOGGER
        << "Stopping connector with timeout = "
        << timeout << std::endl;
  }

  /// @overload Initiator
  void stop(bool force) {
    IBAPI_SOCKET_INITIATOR_LOGGER
        << "Stopping connector with force = "
        << force << std::endl;

    SocketConnectorMap::iterator itr = socketConnectors_.begin();
    for (; itr != socketConnectors_.end(); ++itr) {

      IBAPI_SOCKET_INITIATOR_LOGGER << "Stopping connector (sessionID="
                                    << itr->first << ")";
      bool stopped = (*itr).second->stop();

      IBAPI_SOCKET_INITIATOR_LOGGER << "Stopped connector (sessionID="
                                    << itr->first << "): " << stopped;
    }
  }

  /// @overload Initiator
  bool isLoggedOn() {
    return false;
  }

 private:
  Application& application_;
  std::list<SessionSetting>& sessionSettings_;
  SocketConnector::Strategy& strategy_;
  zmq::context_t* inboundContextPtr_;
  zmq::context_t* outboundContextPtr_;

  typedef std::map< SessionID, boost::shared_ptr<SocketConnector> >
  SocketConnectorMap;
  SocketConnectorMap socketConnectors_;

  SocketConnector::ZmqAddressMap outboundChannels_;

  typedef atp::zmq::Publisher* ChannelPublisher;
  std::map< int, ChannelPublisher > channelPublishers_;

  boost::mutex mutex_;
};


SocketInitiator::SocketInitiator(Application& app,
                                 std::list<SessionSetting>& settings)
    : impl_(new SocketInitiatorImpl(app, settings, *this))
{
}

SocketInitiator::~SocketInitiator()
{
}

void SocketInitiator::startPublisher(int channel, const std::string& address)
    throw ( ConfigError, RuntimeError)
{
  impl_->startPublisher(channel, address);
}

/// @overload Initiator
void SocketInitiator::start() throw ( ConfigError, RuntimeError )
{
  impl_->start();
}

/// @overload Initiator
void SocketInitiator::block() throw ( ConfigError, RuntimeError )
{
  impl_->block();
}

/// @overload Initiator
void SocketInitiator::stop(double timeout)
{
  impl_->stop(timeout);
}

/// @overload Initiator
void SocketInitiator::stop(bool force)
{
  impl_->stop(force);
}

/// @overload Initiator
bool SocketInitiator::isLoggedOn()
{
  return impl_->isLoggedOn();
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onConnect(SocketConnector& connector, int clientId)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connection (" << clientId << ") established.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onData(SocketConnector& connector, int clientId)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connection (" << clientId << ") data.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onError(SocketConnector& connector)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connection error.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onTimeout(SocketConnector& connector)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connection timeout.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onDisconnect(SocketConnector& connector,
                                   int clientId)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connection (" << clientId << ") disconnected.";
}

} // namespace ib

