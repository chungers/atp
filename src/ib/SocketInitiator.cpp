
#include <map>
#include <list>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <Shared/EWrapper.h>

#include "common.hpp"
#include "log_levels.h"
#include "ib/SocketInitiator.hpp"
#include "ib/SessionID.hpp"
#include "zmq/Publisher.hpp"


namespace ib {
namespace internal {

static bool isInProc(const std::string& address)
{
  return boost::starts_with(address, "inproc://");
}

} // namespace internal
} // namespace ib
namespace IBAPI {


bool SocketInitiator::ParseSessionSettingsFromFlag(
    const string& flagValue,
    SocketInitiator::SessionSettings& settings)
{
  vector<string> connectorSpecs;
  boost::split(connectorSpecs, flagValue, boost::is_any_of(","));

  for (vector<string>::iterator spec = connectorSpecs.begin();
       spec != connectorSpecs.end(); ++spec) {

    unsigned int sessionId, port;
    string host, reactor;

    if (SessionSetting::ParseSessionSettingFromString(*spec, &sessionId,
                                                      &host, &port, &reactor)) {
      SessionSetting setting(sessionId, host, port, reactor);
      settings.push_back(setting);
    } else {
      return false;
    }
  }
  return true;
}

bool ParseChannelSettingFromString(const string& input,
                                   unsigned int* channelId,
                                   string* endpoint)
{
  /// format:  {channel_id}={push_endpoint}
  stringstream iss(input);

  string channel_id;
  std::getline(iss, channel_id, '=');
  *channelId = boost::lexical_cast<unsigned int>(channel_id);

  string push_endpoint;
  iss >> push_endpoint;

  endpoint->assign(push_endpoint);
  return true;
}

bool SocketInitiator::ParseOutboundChannelMapFromFlag(
    const string& flagValue,
    map<int, string>& outboundMap)
{
  vector<string> outboundSpecs;
  boost::split(outboundSpecs, flagValue, boost::is_any_of(","));
  for (vector<string>::iterator spec = outboundSpecs.begin();
       spec != outboundSpecs.end(); ++spec) {

    unsigned int channel;
    string endpoint;

    if (ParseChannelSettingFromString(*spec, &channel, &endpoint)) {
      outboundMap[channel] = endpoint;
    } else {
      return false;
    }
  }
  return true;
}

bool SocketInitiator::Configure(SocketInitiator& initiator,
                                map<int, string>& outboundMap,
                                bool publish)
{
  map<int, string>::iterator outboundEndpoint = outboundMap.begin();
  for (; outboundEndpoint != outboundMap.end(); ++outboundEndpoint) {
    int channel = outboundEndpoint->first;
    string endpoint = outboundEndpoint->second;

    if (publish) {
      IBAPI_SOCKET_INITIATOR_LOGGER
          << "Channel " << channel << ", PUBLISH to " << endpoint;
      initiator.publish(channel, endpoint);
    } else {
      IBAPI_SOCKET_INITIATOR_LOGGER
          << "Channel " << channel << ", PUSH to " << endpoint;
      initiator.push(channel, endpoint);
    }
  }
  return true;
}

class SocketInitiatorImpl : public SocketInitiator {

 public :
  SocketInitiatorImpl(Application& app,
                      const SessionSettings& settings,
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
    IBAPI_SOCKET_INITIATOR_LOGGER <<  "Started Initiator.";
  }

  ~SocketInitiatorImpl()
  {
    IBAPI_SOCKET_INITIATOR_LOGGER <<  "Shutting down initiator.";
    stop(true);
    std::map< SocketConnector::ZmqAddress, ChannelPublisher >::iterator pub =
        publishers_.begin();
    for (; pub != publishers_.end(); ++pub) {
      if (pub->second) {
        IBAPI_SOCKET_INITIATOR_LOGGER
            << "Destroying publisher for endpoint "
            << pub->first << " @ " << pub->first;
        delete pub->second;
      }
    }
  }

  /// Starts publisher at the given zmq address.
  void publish(int channel, const std::string& address)
      throw ( ConfigError, RuntimeError )
  {
    if (connectorsRunning_) {
      throw RuntimeError("SocketConnectors are already running.");
    }

    boost::unique_lock<boost::mutex> lock(mutex_);

    if (outboundChannels_.find(channel) == outboundChannels_.end()) {

      // If we are running the publisher, then use in process context
      // for the connector (outbound) and publisher.
      std::stringstream connectorOutboundAddress;
      connectorOutboundAddress << "inproc://channel-" << channel;

      // Check if we have already created this publisher before:
      ChannelPublisher publisher = NULL;
      if (publishers_.find(address) == publishers_.end()) {

        // start publisher
        IBAPI_SOCKET_INITIATOR_LOGGER
            << "Starting embedded publisher for channel " << channel
            << " at " << address;
        publisher = new atp::zmq::Publisher(
            connectorOutboundAddress.str(),
            address, outboundContext_);

        publishers_[address] = publisher;

      } else {

        publisher = publishers_[address];
        IBAPI_SOCKET_INITIATOR_LOGGER
            << "Publisher @ " << address << " already started. "
            << "Sharing " << publisher;
      }

      // Bind the connector's outbound channels to the internal
      // socket address to communicate with the publisher.
      // The SocketConnector will create its own socket inside the
      // appropriate event thread.
      outboundChannels_[channel] = connectorOutboundAddress.str();

    } else {
      IBAPI_SOCKET_INITIATOR_LOGGER
          << "Publisher for channel " << channel << " already running: "
          << outboundChannels_[channel];
    }
  }

  /// Instead of running embedded publisher, push the messages for given
  /// channel to the given endpoint.
  void push(int channel, const std::string& address)
      throw ( ConfigError, RuntimeError )
  {
    if (connectorsRunning_) {
      throw RuntimeError("SocketConnectors are already running.");
    }

    // This is simply a way to set up the channel at the specified
    // outbound address.  There is no need to create sockets since
    // that is managed by the SocketConnector itself.
    if (outboundChannels_.find(channel) == outboundChannels_.end()) {
      outboundChannels_[channel] = address;
      IBAPI_SOCKET_INITIATOR_LOGGER
          << "Channel " << channel << " routed to " << address;
    } else {
      IBAPI_SOCKET_INITIATOR_LOGGER
          << "Channel " << channel << " already routed to "
          << outboundChannels_[channel];
    }
  }

  /// @overload Initiator
  void start(bool connect) throw ( ConfigError, RuntimeError )
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


      // Start the connector
      if (!socketConnector->start()) {
          strategy_.onError(*socketConnector);
          LOG(FATAL) << "Session " << sessionId << " cannot start reactor.";
      }

      // Connect
      unsigned int id = 0;
      if (connect) {
        id = socketConnector->connect(itr->getIp(), itr->getPort(),
                                      sessionId, &strategy_);
      }

      if (id == sessionId) {
        IBAPI_SOCKET_INITIATOR_LOGGER << "Session " << sessionId << " ready: "
                                      << itr->getIp() << ":" << itr->getPort()
                                      << " ==> "
                                      << itr->getConnectorReactorAddress();

      } else {

        if (connect) {
          strategy_.onError(*socketConnector);
          LOG(FATAL) << "Session " << sessionId << " cannot start.";
        }

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

        if (stopped) {
          strategy_.onDisconnect(*(itr->second), itr->first);
        } else {
          strategy_.onError(*(itr->second));
        }
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
  SessionSettings sessionSettings_;
  SocketConnector::Strategy& strategy_;

  zmq::context_t* inboundContext_;
  zmq::context_t* outboundContext_;

  typedef std::map< SessionID, boost::shared_ptr<SocketConnector> >
  SocketConnectorMap;
  SocketConnectorMap socketConnectors_;

  SocketConnector::ZmqAddressMap outboundChannels_;

  typedef atp::zmq::Publisher* ChannelPublisher;
  std::map< SocketConnector::ZmqAddress, ChannelPublisher > publishers_;

  boost::mutex mutex_;
  bool connectorsRunning_;
  boost::condition_variable connectorsRunningCond_;
};


SocketInitiator::SocketInitiator(Application& app,
                                 const SessionSettings& settings)
    : impl_(new SocketInitiatorImpl(app, settings, *this))
{
}

SocketInitiator::~SocketInitiator()
{
}

void SocketInitiator::publish(int channel, const std::string& address)
    throw ( ConfigError, RuntimeError)
{
  impl_->publish(channel, address);
}

void SocketInitiator::push(int channel, const std::string& address)
    throw ( ConfigError, RuntimeError)
{
  impl_->push(channel, address);
}

/// @overload Initiator
void SocketInitiator::start(bool connect) throw ( ConfigError, RuntimeError )
{
  impl_->start(connect);
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
      << "Connector(" << &connector << "): "
      << "Connection (" << clientId << ") established.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onData(SocketConnector& connector, int clientId)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connector(" << &connector << "): "
      << "Connection (" << clientId << ") data.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onError(SocketConnector& connector)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connector(" << &connector << "): "
      << "Connection error.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onTimeout(SocketConnector& connector)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connector(" << &connector << "): "
      << "Connection timeout.";
}

/// @implement SocketConnector::Strategy
void SocketInitiator::onDisconnect(SocketConnector& connector,
                                   int clientId)
{
  IBAPI_SOCKET_INITIATOR_LOGGER
      << "Connector(" << &connector << "): "
      << "Connection (" << clientId << ") disconnected.";
}

} // namespace ib

