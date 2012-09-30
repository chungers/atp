

#include <string>

#include <boost/function.hpp>
#include <boost/unordered_map.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"

#include "test.pb.h"

#include "common/executor.hpp"
#include "common/factory.hpp"
#include "zmq/ZmqUtils.hpp"

using std::string;
using ::zmq::context_t;
using ::zmq::error_t;
using ::zmq::socket_t;
using atp::common::factory;

template <typename Context>
struct Handler
{
  typedef unsigned int status_t;

  static const status_t CANNOT_PARSE    = 100;
  static const status_t NOT_INITIALIZED = 101;

  virtual status_t Execute(Context& context, const string& message) = 0;
};

template <typename ProtoBufferMessage,
          typename Context,
          class Processor =
          boost::function< unsigned int(const ProtoBufferMessage& message,
                                        Context& context) > >
class MessageHandler : Handler<Context>
{
 public:

  typedef unsigned int status_t;

  MessageHandler(Processor processor) : processor_(processor) {}
  virtual ~MessageHandler() {}

  virtual status_t Execute(Context& context,
                           const string& message)
  {
    message_.Clear();

    if (message_.ParseFromString(message)) {
      if (message_.IsInitialized()) {
        return processor_(message_, context);
      } else {
        return Handler<Context>::NOT_INITIALIZED;
      }
    } else {
      return Handler<Context>::CANNOT_PARSE;
    }
  }

 private:
  ProtoBufferMessage message_;
  Processor processor_;
};

/// Basic message listener that handles the message based on
/// a topic followed by a protobuffer string
template <typename Context>
class MessageListener
{
 public:

  MessageListener(const string& endpoint, context_t* context,
                  factory<Handler<Context> >& factory) :
      endpoint_(endpoint),
      context_(context),
      factory_(factory)
  {
  }

 private:

  string endpoint_;
  context_t* context_;
  factory< Handler<Context> >& factory_;
};

class Agent
{

};



TEST(AgentPrototype, Test1)
{
}

