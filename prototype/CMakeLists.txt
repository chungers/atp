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
set(prototype_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
)
set(prototype_srcs
  ${TEST_DIR}/AllTests.cpp
  numeric_prototype.cpp
)
set(prototype_libs
  api_base
  ib_api_versioned_base
  atp_log_reader
  atp_message_processor
  atp_proto
  atp_zmq
  boost_system
  boost_iostreams
  test_protos
  gflags
  glog
  protobuf
  ta_lib
  bz2
  z
  )
cpp_gtest(prototype)

# Prototype
set(prototype_indicator_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
)
set(prototype_indicator_srcs
  ${TEST_DIR}/AllTests.cpp
  indicator_prototype.cpp
)
set(prototype_indicator_libs
  api_base
  ib_api_versioned_base
  atp_log_reader
  atp_message_processor
  atp_proto
  atp_zmq
  boost_system
  boost_iostreams
  test_protos
  gflags
  glog
  protobuf
  ta_lib
  bz2
  z
  )
cpp_gtest(prototype_indicator)

# prototype_strategy
set(prototype_strategy_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(prototype_strategy_srcs
  ${TEST_DIR}/AllTests.cpp
  agent_prototype.cpp
)
set(prototype_strategy_libs
  atp_message_processor
  atp_log_reader
  atp_proto
  atp_zmq
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
cpp_gtest(prototype_strategy)

# prototype_data_structures
set(prototype_data_structures_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(prototype_data_structures_srcs
  ${TEST_DIR}/AllTests.cpp
  misc_prototype.cpp
)
set(prototype_data_structures_libs
  atp_message_processor
  atp_log_reader
  atp_proto
  atp_zmq
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
cpp_gtest(prototype_data_structures)

add_custom_target(all_prototypes)
add_dependencies(all_prototypes
  prototype
  prototype_data_structures
  prototype_indicator
  prototype_strategy
)
