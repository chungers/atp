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
  gflags
  glog
  zmq
)

cpp_library(ZmqComponents)

