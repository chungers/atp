#
# Tests for platform code

message(STATUS "TESTS: Platform ==============================================")

# test_platform_message_processor
set(test_platform_message_processor_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_platform_message_processor_srcs
  ${TEST_DIR}/AllTests.cpp
  MessageProcessorTest.cpp
)
set(test_platform_message_processor_libs
  atp_platform_all
  gflags
  )
cpp_gtest(test_platform_message_processor)


# test_platform_marketdata_handler
set(test_platform_marketdata_handler_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_platform_marketdata_handler_srcs
  ${TEST_DIR}/AllTests.cpp
  MarketDataHandlerTest.cpp
)
set(test_platform_marketdata_handler_libs
  atp_platform_all
  gflags
  )
cpp_gtest(test_platform_marketdata_handler)

# test_platform_indicator_pipeline
set(test_platform_indicator_pipeline_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_platform_indicator_pipeline_srcs
  ${TEST_DIR}/AllTests.cpp
  IndicatorPipelineTest.cpp
)
set(test_platform_indicator_pipeline_libs
  atp_platform_all
  gflags
  )
cpp_gtest(test_platform_indicator_pipeline)

add_custom_target(all_platform_tests)
add_dependencies(all_platform_tests
  test_platform_message_processor
  test_platform_marketdata_handler
  test_platform_indicator_pipeline
)
