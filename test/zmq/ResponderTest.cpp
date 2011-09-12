
#include <string>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "common.hpp"
#include "zmq/Responder.hpp"
#include "zmq/ZmqUtils.hpp"


using namespace atp::zmq;

typedef std::vector<std::string> Message;
#define LOGGER VLOG(20)


struct TestReader : SocketReader
{
  TestReader(size_t howMany = 1) : howMany(howMany) {}
  bool operator()(zmq::socket_t& socket)
  {
    Message message;
    std::string buff;
    bool status = false;
    try {
      LOGGER << "TestReader about to receive on socket." << std::endl;
      while (atp::zmq::receive(socket, &buff)) {
        message.push_back(buff);
      }
      // Last line
      message.push_back(buff);
      boost::lock_guard<boost::mutex> lock(mutex);
      messages.push_back(message);
      LOGGER << "Total messages received: " << messages.size() << std::endl;

      if (messages.size() == howMany) {
        LOGGER << "Notifying blockers." << std::endl;
        hasReceivedMessage.notify_one();
      }
      status = true;
    } catch (zmq::error_t e) {
      LOG(WARNING) << "Got exception " << e.what() << std::endl;
      status = false;
    }
    return status;
  }

  size_t howMany;
  std::vector<Message> messages;
  boost::mutex mutex;
  boost::condition_variable hasReceivedMessage;
};

struct TestWriter : SocketWriter {

  bool operator()(zmq::socket_t& socket)
  {
    std::string ok("OK");
    size_t sent = atp::zmq::send_zero_copy(socket , ok);
    LOGGER << "Sent reply" << std::endl;
    return sent == ok.length();
  }
};

TEST(ResponderTest, SimpleIPCSendReceiveTest)
{
  LOGGER << "Current TimeMicros = " << now_micros() << std::endl;

  int messages = 1;
  TestReader testReader(messages);
  TestWriter testWriter;

  const std::string& addr = atp::zmq::EndPoint::ipc("test1");
  // Immediately starts a listening thread at the given address.
  Responder responder(addr, testReader, testWriter);

  // For inproc endpoint, we need to use a shared context. Otherwise, the
  // program will crash.
  zmq::context_t context(1);
  zmq::socket_t client(context, ZMQ_REQ);
  client.connect(addr.c_str());

  std::ostringstream oss;
  oss << "This is a test";

  std::string message = oss.str();  // copies the return value of oss
  atp::zmq::send_zero_copy(client, message);

  // Waiting for the other side to receive it.
  boost::unique_lock<boost::mutex> lock(testReader.mutex);
  testReader.hasReceivedMessage.wait(lock);

  Message received = testReader.messages.front();
  ASSERT_EQ(message, received[0]);
  ASSERT_EQ(messages, testReader.messages.size());

  LOGGER << "Checking the response" << std::endl;
  // Read the response
  std::string response;
  atp::zmq::receive(client, &response);
  ASSERT_EQ("OK", response);
  LOGGER << "Response was " << response << std::endl;

  client.close();
}

TEST(ResponderTest, IPCMultiSendReceiveTest)
{
  LOGGER << "Current TimeMicros = " << now_micros() << std::endl;

  int messages = 5000;
  TestReader testReader(messages);
  TestWriter testWriter;

  const std::string& addr = atp::zmq::EndPoint::ipc("test2");

  Responder responder(addr, testReader, testWriter);

  sleep(1);

  LOGGER << "Starting client." << std::endl;
  zmq::context_t context(1);
  zmq::socket_t client(context, ZMQ_REQ);
  bool exception = false;
  try {
    client.connect(addr.c_str());
  } catch (zmq::error_t e) {
    LOG(ERROR) << "Exception: " << e.what() << std::endl;
    exception = true;
  }
  ASSERT_FALSE(exception);
  LOGGER << "Client connected." << std::endl;

  std::vector<std::string> sent;
  for (int i = 0; i < messages; ++i) {
    std::ostringstream oss;
    oss << "Message-" << i;

    std::string message(oss.str());
    bool exception = false;
    try {
      LOGGER << "Sending message " << message << std::endl;
      atp::zmq::send_zero_copy(client, message);

      std::string reply;
      atp::zmq::receive(client, &reply);

      LOGGER << "Reply = " << reply << std::endl;
      ASSERT_EQ("OK", reply);

    } catch (zmq::error_t e) {
      LOG(ERROR) << "Exception: " << e.what() << std::endl;
      exception = true;
    }
    ASSERT_FALSE(exception);
    sent.push_back(message);
  }

  LOGGER << "Finished sending." << std::endl;

  EXPECT_EQ(messages, testReader.messages.size());

  int i = 0;
  for (std::vector<Message>::iterator itr = testReader.messages.begin();
       itr != testReader.messages.end();
       ++itr) {
    EXPECT_EQ(sent[i++], itr->at(0));
  }

}

