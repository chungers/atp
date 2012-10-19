
message(STATUS "Historian ====================================================")

add_definitions('-fPIC')

##########################################
# Historian
set(historian_components_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(historian_components_srcs
  Db.cpp
  DbReactorStrategy.cpp
  DbReactorClient.cpp
)
set(historian_components_libs
  proto_components
  leveldb
  zmq_components
  boost_date_time
)
cpp_library(historian_components)

