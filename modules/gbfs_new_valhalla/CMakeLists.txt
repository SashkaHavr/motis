cmake_minimum_required(VERSION 3.10)
project(motis)

file(GLOB_RECURSE motis-gbfs_new_valhalla-files src/*.cc)
add_library(motis-gbfs_new_valhalla STATIC ${motis-gbfs_new_valhalla-files})
target_include_directories(motis-gbfs_new_valhalla PUBLIC include)
target_compile_features(motis-gbfs_new_valhalla PRIVATE cxx_std_20)
target_link_libraries(motis-gbfs_new_valhalla
  motis-module
  motis-core
  geo
  motis-libjson
  http-client
)
target_compile_options(motis-gbfs_new_valhalla PRIVATE ${MOTIS_CXX_FLAGS})
