# Boost iostreams
#
message(STATUS "Boost iostreams ==============================================")

add_definitions('-fPIC')

##########################################
# boost iostreams
set(boost_iostreams_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
FILE(GLOB IMPL_FILES "${SRC_DIR}/boost/iostreams/*.cpp")
set(boost_iostreams_srcs
  ${IMPL_FILES}
)
set(boost_iostreams_libs
  boost_system
  boost_thread
)
cpp_library(boost_iostreams)
