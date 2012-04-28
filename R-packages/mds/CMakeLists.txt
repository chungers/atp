# R package - mds
# historian client for R
#
message(STATUS "R-packages: mds ==============================================")
message(STATUS "OS: ${CMAKE_SYSTEM_NAME}")

add_definitions('-fPIC')

# Generate raptor R module MakeVars
configure_file(
  ./src/Makevars.tpl
  ./src/Makevars
)

set(R_PKG "mds")
set(R_PKG_DIR "${R_PKGS_DIR}/mds")
set(R_LIBS "${R_PKG_DIR}/${CMAKE_SYSTEM_NAME}")

set(R_BUNDLE "${R_PKG}_1.0.tar.gz")

add_custom_target(r_clean_mds
COMMAND rm -rf ${R_LIBS} & rm -f ./src/*.pb.* ./src/*.o ./src/*.so
COMMENT "Removing directory ${R_LIBS}",
)

add_custom_target(r_mkdirs_mds
COMMAND mkdir -p ${R_LIBS}
COMMENT "Making directory ${R_LIBS}",
)

add_custom_target(r_gen_protos_mds
COMMAND protoc --proto_path=proto --cpp_out=${R_PKG_DIR}/src proto/*.proto
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Building proto buffs",
)

add_custom_target(r_build_mds
COMMAND R CMD build ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Building R package ${R_PKG}",
)

add_custom_target(r_install_mds
COMMAND R CMD INSTALL --library=${R_LIBS} --no-multiarch ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Compile / install R package ${R_PKG}",
)

add_custom_target(r_package_mds
COMMAND R CMD INSTALL --build --no-multiarch --no-inst ${R_PKG}
WORKING_DIRECTORY ${R_PKGS_DIR}
COMMENT "Creating precompiled binary package ${R_PKG}",
)

add_dependencies(r_build_mds
  zmq_components
  varz_components
  historian_components
  proto_components
)

add_dependencies(r_install_mds r_build_mds r_gen_protos_mds r_mkdirs_mds)
add_dependencies(r_package_mds r_install_mds)
