
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
      strategy_(strategy),
      inboundContext_(NULL),
      outboundContext_(NULL),
      connectorsRunning_(false)
  {
    if (outboundContext_ == NULL) {
      outboundContext_ = new zmq::context_t(1);
    }
  }

  ~SocketInitiatorImpl()
  {
    IBAPI_SOCKET_INITIATOR_LOGGER <<  "Shutting down initiator.";
    stop(true);
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
          address, outboundContext_);

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

    // Start up the socket connectors one by one.
    std::list<SessionSetting>::iterator itr;
    for (itr = sessionSettings_.begin();
         itr != sessionSettings_.end();
         ++itr) {

      SessionID sessionId = static_cast<SessionID>(itr->getConnectionId());

      if (socketConnectors_.find(sessionId) != socketConnectors_.end()) {
        IBAPI_SOCKET_INITIATOR_LOGGER
            << "Connector " << sessionId << ": "
            << "Already exists " << socketConnectors_[sessionId];
        continue;
      }

      IBAPI_SOCKET_INITIATOR_LOGGER
          << "Starting connector "
          << itr->getConnectionId()
          << ", inbound: " << itr->getConnectorReactorAddress()
          << ", inboundContext: " << inboundContext_
          << ", outboundContext: " << outboundContext_;

      boost::shared_ptr<SocketConnector> socketConnector =
          boost::shared_ptr<SocketConnector>(
              new SocketConnector(itr->getConnectorReactorAddress(),
                                  outboundChannels_,
                                  application_, sessionId,
                                  inboundContext_,
                                  outboundContext_));

      // Register this in a map for clean up later.
      socketConnectors_[sessionId] = socketConnector;

      IBAPI_SOCKET_INITIATOR_LOGGER << "SocketConnector: " << *itr;
      int id = socketConnector->connect(itr->getIp(), itr->getPort(),
                                        sessionId, &strategy_);

      if (id == sessionId) {
        IBAPI_SOCKET_INITIATOR_LOGGER << "Session " << sessionId << " ready: "
                                      << itr->getIp() << ":" << itr->getPort()
                                      << " ==> "
                                      << itr->getConnectorReactorAddress();
      } else {
        LOG(FATAL) << "Session " << sessionId << " cannot start.";
      }
      sleep(1); // Wait a bit.
    }

    // Set the condition
    connectorsRunning_ = true;
    connectorsRunningCond_.notify_all();
  }

  /// @overload Initiator
  void block() throw ( ConfigError, RuntimeError )
  {
    IBAPI_SOCKET_INITIATOR_LOGGER
        << "Begin blocking while connectors are running.";
    boost::unique_lock<boost::mutex> lock(mutex_);
    while (connectorsRunning_) {
     connectorsRunningCond_.wait(lock);
    }
    IBAPI_SOCKET_INITIATOR_LOGGER
        << "End blocking";
  }

  /// @overload Initiator
  void stop(bool force) {
    boost::unique_lock<boost::mutex> lock(mutex_);

    IBAPI_SOCKET_INITIATOR_LOGGER
        << "Stopping connector with force = "
        << force << std::endl;

    if (connectorsRunning_) {
      SocketConnectorMap::iterator itr = socketConnectors_.begin();
      for (; itr != socketConnectors_.end(); ++itr) {

        bool stopped = (*itr).second->stop();
        IBAPI_SOCKET_INITIATOR_LOGGER << "Stopped connector (sessionID="
                                      << itr->first << "): " << stopped;
        sleep(1); // Wait a bit.
      }

      // Set the condition
      connectorsRunning_ = false;
      connectorsRunningCond_.notify_all();
    }
  }

  /// @overload Initiator
  bool isLoggedOn() {
    return connectorsRunning_;
  }

 private:
  Application& application_;
  std::list<SessionSetting>& sessionSettings_;
  SocketConnector::Strategy& strategy_;
  zmq::context_t* inboundContext_;
  zmq::context_t* outboundContext_;

  typedef std::map< SessionID, boost::shared_ptr<SocketConnector> >
  SocketConnectorMap;
  SocketConnectorMap socketConnectors_;

  SocketConnector::ZmqAddressMap outboundChannels_;

  typedef atp::zmq::Publisher* ChannelPublisher;
  std::map< int, ChannelPublisher > channelPublishers_;

  boost::mutex mutex_;
  bool connectorsRunning_;
  boost::condition_variable connectorsRunningCond_;
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

