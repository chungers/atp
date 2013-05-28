#
# Tests for common code

message(STATUS "TESTS: Time Series ===========================================")

# Use standard string library instead of mvc foundation classes:
option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

# MovingWindow
set(test_time_series_moving_window_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_time_series_moving_window_srcs
  ${TEST_DIR}/AllTests.cpp
  MovingWindowTest.cpp
 )
set(test_time_series_moving_window_libs
  atp_common
  atp_proto
  atp_zmq
  gflags
  glog
  )
cpp_gtest(test_time_series_moving_window)

# TimeSeries
set(test_time_series_time_series_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_time_series_time_series_srcs
  ${TEST_DIR}/AllTests.cpp
  TimeSeriesTest.cpp
 )
set(test_time_series_time_series_libs
  atp_common
  atp_proto
  atp_zmq
  gflags
  glog
  )
cpp_gtest(test_time_series_time_series)

add_custom_target(all_time_series_tests)
add_dependencies(all_time_series_tests
  test_time_series_moving_window
  test_time_series_time_series
)
