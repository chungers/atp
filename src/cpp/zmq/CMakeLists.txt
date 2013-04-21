# Zmq
#

message(STATUS "ZeroMQ =======================================================")

add_definitions('-fPIC')

##########################################
# Zmq library
set(atp_zmq_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_zmq_srcs
  Reactor.cpp
  Publisher.cpp
  Subscriber.cpp
)
set(atp_zmq_libs
  boost_system
  boost_thread
  gflags
  glog
  atp_varz
  zmq
)
cpp_library(atp_zmq)

