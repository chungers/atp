
# IB client
add_subdirectory(ib)

# Historian client
add_subdirectory(hzc)

# Marketdata Subscriber
add_subdirectory(mds)

add_custom_target(all_R_packages)
add_dependencies(all_R_packages
  ib_api_versioned
  r_package_ib
  r_package_hzc
#  r_package_mds
)

add_custom_target(clean_all_R_packages)
add_dependencies(clean_all_R_packages
  r_clean_ib
  r_clean_hzc
#  r_clean_mds
)

add_custom_target(deploy_all_R_packages)
add_dependencies(deploy_all_R_packages
  r_deploy_ib
  r_deploy_hzc
#  r_deploy_mds
)