# Zmq
#

##########################################
# Zmq library
set(ZmqComponents_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(ZmqComponents_srcs
  Reactor.cpp
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
# Reactor test client / server
set(zmq_reactor_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(zmq_reactor_srcs
  reactor_main.cpp
)
set(zmq_reactor_libs
  boost_system
  gflags
  glog
  ZmqComponents
)
cpp_executable(zmq_reactor)
