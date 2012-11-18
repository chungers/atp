#
# Tests for common code

message(STATUS "TESTS: Prototypes ============================================")

# Use standard string library instead of mvc foundation classes:
option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)


##########################################
# prototyping
proto_library(test_protos test.proto ${SRC_DIR})


# Prototype
set(Prototypes_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
)
set(Prototypes_srcs
  ${TEST_DIR}/AllTests.cpp
  indicator_prototype.cpp
  misc_prototype.cpp
)
set(Prototypes_libs
  api_base
  ib_api_versioned_base
  proto_components
  zmq_components
  gflags
  glog
  test_protos
  protobuf
  protobuf-lite
  ta_lib
  )
cpp_gtest(Prototypes)


# StrategyPrototype
set(StrategyPrototype_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(StrategyPrototype_srcs
  ${TEST_DIR}/AllTests.cpp
  agent_prototype.cpp
)
set(StrategyPrototype_libs
  message_processor
  log_reader
  proto_components
  zmq_components
  boost_system
  boost_iostreams
  gflags
  glog
  test_protos
  protobuf
  ta_lib
  bz2
  z
  )
cpp_gtest(StrategyPrototype)


add_custom_target(all_prototypes)
add_dependencies(all_prototypes
  Prototypes
  StrategyPrototype
)
