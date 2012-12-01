# R package - ib
# historian client for R
#
message(STATUS "R-packages: ib ===============================================")
message(STATUS "OS: ${CMAKE_SYSTEM_NAME}")

add_definitions('-fPIC')

# Generate raptor R module MakeVars
configure_file(
  ./src/Makevars.tpl
  ./src/Makevars
)

set(R_PKG "ib")
set(R_PKG_DIR "${R_PKGS_DIR}/ib")
set(R_LIBS "${R_PKG_DIR}/${CMAKE_SYSTEM_NAME}")

set(R_BUNDLE "${R_PKG}_1.0.tar.gz")

add_custom_target(r_clean_ib
COMMAND rm -rf ${R_LIBS} & rm -f ./src/*.pb.* ./src/*.o ./src/*.so
COMMENT "Removing directory ${R_LIBS}",
)

add_custom_target(r_mkdirs_ib
COMMAND mkdir -p ${R_LIBS}
COMMENT "Making directory ${R_LIBS}",
)

add_custom_target(r_gen_protos_ib
COMMAND protoc --proto_path=proto --cpp_out=${R_PKG_DIR}/src proto/*.proto
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Building proto buffs",
)

add_custom_target(r_bundle_ib
COMMAND R CMD build ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Building R package ${R_PKG}",
)

add_custom_target(r_build_ib
COMMAND R CMD INSTALL --library=${R_LIBS} --no-multiarch ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Compile / install R package ${R_PKG}",
)

add_dependencies(r_bundle_ib
  ib_api
  api_base
  api_versioned
  atp_zmq
  atp_proto
)

add_custom_target(r_package_ib
COMMAND R CMD INSTALL --build --no-multiarch --no-inst ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Creating precompiled binary package ${R_PKG}",
)

add_custom_target(r_deploy_ib
COMMAND R CMD INSTALL --no-multiarch ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Install into R locally"
)

add_dependencies(r_build_ib r_bundle_ib r_gen_protos_ib r_mkdirs_ib)
add_dependencies(r_package_ib r_build_ib)
add_dependencies(r_deploy_ib r_package_ib)
