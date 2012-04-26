# R package - hzc
# historian client for R
#
message(STATUS "R-packages: hzc ==============================================")
message(STATUS "OS: ${CMAKE_SYSTEM_NAME}")

add_definitions('-fPIC')

# Generate raptor R module MakeVars
configure_file(
  ./src/Makevars.tpl
  ./src/Makevars
)

set(R_PKG "hzc")
set(R_PKG_DIR "${R_PKGS_DIR}/hzc")
set(R_LIBS "${R_PKG_DIR}/${CMAKE_SYSTEM_NAME}")

set(R_BUNDLE "${R_PKG}_1.0.tar.gz")

add_custom_target(r_clean_hzc
COMMAND rm -rf ${R_LIBS} & rm -f ./src/*.pb.* ./src/*.o ./src/*.so
COMMENT "Removing directory ${R_LIBS}",
)

add_custom_target(r_mkdirs_hzc
COMMAND mkdir -p ${R_LIBS}
COMMENT "Making directory ${R_LIBS}",
)

add_custom_target(r_gen_protos_hzc
COMMAND protoc --proto_path=proto --cpp_out=${R_PKG_DIR}/src proto/*.proto
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Building proto buffs",
)

add_custom_target(r_build_hzc
COMMAND R CMD build ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Building R package ${R_PKG}",
)

add_custom_target(r_install_hzc
COMMAND R CMD INSTALL --library=${R_LIBS} --no-multiarch ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Compile / install R package ${R_PKG}",
)

add_dependencies(r_build_hzc
  zmq_components
  historian_components
  proto_components
)

add_dependencies(r_install_hzc r_build_hzc r_gen_protos_hzc r_mkdirs_hzc)
