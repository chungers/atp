
#include "zmq/ZmqUtils.hpp"
#include "varz/varz.hpp"

#include "ib/AbstractSocketConnectorReactor.hpp"

DEFINE_VARZ_int64(socket_connector_inbound_requests, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests_ok, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests_errors, 0, "");
DEFINE_VARZ_int64(socket_connector_inbound_requests_exceptions, 0, "");


namespace ib {
namespace internal {

bool AbstractSocketConnectorReactor::respond(zmq::socket_t& socket)
{
  if (!strategy_.IsReady()) {
    return true; // No-op -- don't read the messages from socket yet.
  }

  // Now we are connected.  Process the received messages.
  try {

    while (1) {

      std::string messageKeyFrame;
      bool more = atp::zmq::receive(socket, &messageKeyFrame);
      bool supported = strategy_.IsMessageSupported(messageKeyFrame);

      if (!supported) {

        IBAPI_SOCKET_CONNECTOR_ERROR << "Unsupported message received: "
                                     << messageKeyFrame;
        // keep reading to consume the extra frames:
        while (more) {
          std::string buff;
          more = atp::zmq::receive(socket, &buff);
          IBAPI_SOCKET_CONNECTOR_ERROR << "Unsupported message received: "
                                       << messageKeyFrame
                                       << ", extra frame: " << buff;
        }

        ZmqMessagePtr empty;
        AfterMessage(404, socket, empty);

        // now skip this loop
        continue;
      }

        ZmqMessagePtr inboundMessage;
        int responseCode = 500;

        // Message is supported.  Now create the message and delegate it
        // to the actual reading and parsing from the socket.
        if (more) {

          LOG(INFO) << "Received message of type " << messageKeyFrame;

          strategy_.CreateMessage(messageKeyFrame, inboundMessage);

          if (inboundMessage && (*inboundMessage)->receive(socket)) {

            VARZ_socket_connector_inbound_requests++;

            if (!(*inboundMessage)->validate()) {

              VARZ_socket_connector_inbound_requests_errors++;

              responseCode = 412; // pre conditional failed

              IBAPI_SOCKET_CONNECTOR_ERROR
                  << "Handle inbound message failed: "
                  << inboundMessage;

            } else {

              if (strategy_.Process(inboundMessage)) {

                VARZ_socket_connector_inbound_requests_ok++;

                responseCode = 200;

              } else {

                VARZ_socket_connector_inbound_requests_errors++;

                responseCode = 502; // bad gateway

                // TODO: figure out a better way to log something
                // helpful - like a message type identifier.
                IBAPI_SOCKET_CONNECTOR_ERROR
                    << "Handle inbound message failed: "
                    << inboundMessage;

              }
            }
          } else {
            responseCode = (inboundMessage) ?
                400 : // bad request
                503; // service unavailable
          }
        } // if more

        // Process response after message handled.
        AfterMessage(responseCode, socket, inboundMessage);

        if (strategy_.IsTerminate(inboundMessage)) {
          LOG(INFO) << "Inbound reactor terminating on "
                    << messageKeyFrame;

          return false;
        }
      }
    } catch (zmq::error_t e) {

      VARZ_socket_connector_inbound_requests_exceptions++;

      LOG(ERROR) << "Got exception while handling reactor inbound message: "
                 << e.what();

      return false; // stop processing
    }
    return true; // continue processing
}


} // internal
} // ib

