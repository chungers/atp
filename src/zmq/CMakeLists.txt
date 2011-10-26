# Zmq
#

##########################################
# Zmq library
set(ZmqComponents_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(ZmqComponents_srcs
  Responder.cpp
)
set(ZmqComponents_libs
  boost_system
  boost_thread
  gflags
  glog
  zmq
)
cpp_library(ZmqComponents)

##########################################
# Responder test client / server
set(zmq_responder_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(zmq_responder_srcs
  responder_main.cpp
)
set(zmq_responder_libs
  boost_system
  gflags
  glog
  ZmqComponents
)
cpp_executable(zmq_responder)
