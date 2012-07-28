
add_definitions('-fPIC')


##########################################
# Firehose - gateway to zmq bridge
set(firehose_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(firehose_srcs
  firehose_main.cpp
)
set(firehose_libs
  IBAPIConnector
  ib_api_versioned
  boost_system
  gflags
  glog
)
cpp_executable(firehose)

##########################################
# EM - Execution Manager
set(em_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(em_srcs
  em_main.cpp
)
set(em_libs
  IBAPIConnector
  ib_api_versioned
  boost_system
  gflags
  glog
)
cpp_executable(em)

##########################################
# Marketdata subscriber
set(marketdata_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(marketdata_srcs
  marketdata_main.cpp
)
set(marketdata_libs
  IBAPIConnector
  ib_api_versioned
  boost_system
  gflags
  glog
)
cpp_executable(marketdata)

##########################################
# Simple Log File Reader
set(logreader_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${IBAPI_IMPL_DIR}
)
set(logreader_srcs
  logreader_main.cpp
)
set(logreader_libs
  IBAPIConnector
  ib_api_versioned
  boost_system
  boost_iostreams
  gflags
  glog
  historian_components
  protobuf-lite
  leveldb
  bz2
  z
)
cpp_executable(logreader)


##########################################
# Log File to Db loader
set(hloader_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(hloader_srcs
  historian_loader_main.cpp
)
set(hloader_libs
  IBAPIConnector
  ib_api_versioned
  boost_system
  boost_iostreams
  gflags
  glog
  historian_components
  protobuf-lite
  leveldb
  bz2
  z
)
cpp_executable(hloader)


##########################################
# Historian - DB command line client
set(hdb_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(hdb_srcs
  hdb_main.cpp
)
set(hdb_libs
  gflags
  glog
  historian_components
  varz_components
  zmq_components
)
cpp_executable(hdb)


##########################################
# Historian - DB ZMQ reactor
set(hz_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(hz_srcs
  hz_main.cpp
)
set(hz_libs
  gflags
  glog
  historian_components
  varz_components
  zmq_components
)
cpp_executable(hz)

##########################################
# Historian - DB ZMQ reactor client
set(hzc_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(hzc_srcs
  hzc_main.cpp
)
set(hzc_libs
  gflags
  glog
  historian_components
  varz_components
  zmq_components
)
cpp_executable(hzc)

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
  zmq_components
)
cpp_executable(zmq_reactor)


add_custom_target(all_mains)
add_dependencies(all_mains
  firehose
  logreader
  marketdata
  hloader
  hdb
  hz
  hzc
  zmq_reactor
)