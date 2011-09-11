
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

struct TestReader : SocketReader
{
  TestReader(size_t howMany = 1) : howMany(howMany) {}
  bool operator()(zmq::socket_t& socket)
  {
    Message message;
    std::string buff;
    bool status = false;
    try {
      LOG(INFO) << "TestReader about to receive on socket." << std::endl;
      while (atp::zmq::receive(socket, &buff)) {
        message.push_back(buff);
      }
      // Last line
      message.push_back(buff);
      boost::lock_guard<boost::mutex> lock(mutex);
      messages.push_back(message);
      if (messages.size() == howMany) {
        hasReceivedMessage.notify_one();
      }
      LOG(INFO) << "Total messages received: " << messages.size() << std::endl;
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
    size_t sent = atp::zmq::zero_copy_send(socket , ok);
    LOG(INFO) << "Sent reply" << std::endl;
    return sent == ok.length();
  }
};

TEST(ResponderTest, SimpleInProcSendReceiveTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() << std::endl;

  int messages = 1;
  TestReader testReader(messages);
  TestWriter testWriter;

  const std::string& addr = "inproc://test1";
  // Immediately starts a listening thread at the given address.
  Responder responder(addr, testReader, testWriter);

  // For inproc endpoint, we need to use a shared context. Otherwise, the
  // program will crash.
  zmq::socket_t client(responder.context(), ZMQ_REQ);
  client.connect(addr.c_str());

  std::ostringstream oss;
  oss << "This is a test";

  std::string message = oss.str();  // copies the return value of oss
  atp::zmq::zero_copy_send(client, message);

  // Waiting for the other side to receive it.
  boost::unique_lock<boost::mutex> lock(testReader.mutex);
  testReader.hasReceivedMessage.wait(lock);

  Message received = testReader.messages.front();
  ASSERT_EQ(message, received[0]);
  ASSERT_EQ(messages, testReader.messages.size());

  LOG(INFO) << "Checking the response" << std::endl;
  // Read the response
  std::string response;
  atp::zmq::receive(client, &response);
  ASSERT_EQ("OK", response);
  LOG(INFO) << "Response was " << response << std::endl;

  LOG(INFO) << "Stopping the responder." << std::endl;
  responder.stop();
}

TEST(ResponderTest, InProcSendReceiveTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() << std::endl;

  int messages = 1000;
  TestReader testReader(messages);
  TestWriter testWriter;

  const std::string& addr = "inproc://test2";
  // Immediately starts a listening thread at the given address.
  Responder responder(addr, testReader, testWriter);

  zmq::socket_t client(responder.context(), ZMQ_REQ);
  client.connect(addr.c_str());

  std::vector<std::string> sent;
  for (int i = 0; i < messages; ++i) {
    std::ostringstream oss;
    oss << "Message-" << i;

    std::string message = oss.str();  // copies the return value of oss
    atp::zmq::zero_copy_send(client, message);

    sent.push_back(message);
  }

  // Waiting for the other side to receive it.
  boost::unique_lock<boost::mutex> lock(testReader.mutex);
  testReader.hasReceivedMessage.wait(lock);

  responder.stop();

  EXPECT_EQ(messages, testReader.messages.size());

  int i = 0;
  for (std::vector<Message>::iterator itr = testReader.messages.begin();
       itr != testReader.messages.end();
       ++itr) {
    EXPECT_EQ(sent[i++], itr->at(0));
  }

}

