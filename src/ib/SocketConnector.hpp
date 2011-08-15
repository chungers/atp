#ifndef IBAPI_SOCKET_CONNECTOR_H_
#define IBAPI_SOCKET_CONNECTOR_H_



namespace IBAPI {

/// Models after the SocketConnector in the QuickFIX API.
/// https://github.com/lab616/third_party/blob/master/quickfix-1.13.3/src/C++/SocketConnector.h
class SocketConnector {

 public:
  class Strategy;

  SocketConnector(int timeout = 0);
  ~SocketConnector();

  int connect(const std::string& host, unsigned int port, unsigned int clientId,
              Strategy* s);
  
 private :
  int timeout_;

 public:
  class Strategy {
   public:
    virtual ~Strategy() = 0;
    virtual void onConnect(SocketConnector&, int clientId) = 0;
    virtual void onData(SocketConnector&, int clientId) = 0;
    virtual void onDisconnect(SocketConnector&, int clientId) = 0;
    virtual void onError(SocketConnector&, const int clientId,
                         const unsigned int errorCode) = 0;
    virtual void onTimeout(const long time) = 0;
  };
};


} // namespace IBAPI


#endif // IBAPI_SOCKET_CONNECTOR_H_

