# R package - hzc
# historian client for R
#
message(STATUS "R-packages: hzc ==============================================")
message(STATUS "OS: ${CMAKE_SYSTEM_NAME}")

add_definitions('-pg -g -W -ggdb -Wall -Wno-switch -fPIC -O2')


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

add_custom_target(r_bundle_hzc
COMMAND R CMD build ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Building R package ${R_PKG}",
)

add_custom_target(r_build_hzc
COMMAND R CMD INSTALL --library=${R_LIBS} --no-multiarch ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Compile / install R package ${R_PKG}",
)

add_dependencies(r_bundle_hzc
  atp_zmq
  atp_historian
  atp_proto
)

add_custom_target(r_package_hzc
COMMAND R CMD INSTALL --build --no-multiarch --no-inst ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Creating precompiled binary package ${R_PKG}",
)

add_custom_target(r_deploy_hzc
COMMAND R CMD INSTALL --no-multiarch ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Install into R locally"
)

add_dependencies(r_build_hzc r_bundle_hzc r_gen_protos_hzc r_mkdirs_hzc)
add_dependencies(r_package_hzc r_build_hzc)
add_dependencies(r_deploy_hzc r_package_hzc)
