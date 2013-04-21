
message(STATUS "Historian ====================================================")

add_definitions('-fPIC')

##########################################
# Historian
set(atp_historian_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_historian_srcs
  Db.cpp
  DbReactorStrategy.cpp
  DbReactorClient.cpp
)
set(atp_historian_libs
  atp_common
  atp_proto
  leveldb
  atp_zmq
  boost_date_time
)
cpp_library(atp_historian)

