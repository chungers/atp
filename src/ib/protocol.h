#ifndef IB_PROTOCOL_H_
#define IB_PROTOCOL_H_

// This file has some high-level protocol definitions related
// to how the client and servers interact.


#ifdef SOCKET_CONNECTOR_USES_BLOCKING_REACTOR
#define SOCKET_CONNECTOR_IMPL ib::internal::BlockingReactorImpl
#else
#define SOCKET_CONNECTOR_IMPL ib::internal::NonBlockingReactorImpl
#endif



#endif // IB_PROTOCOL_H_
