cmake_minimum_required(VERSION 3.13)
project(crumbs_native_example C)

# Build this example in one of two ways:
# 1) in-tree: build inside the CRUMBS repo and link to the `crumbs` target
#    cmake -S examples/linux/simple_native_controller -B build -DCRUMBS_BUILD_IN_TREE=ON
# 2) out-of-tree (installed library): set CMAKE_PREFIX_PATH to an installed
#    path that contains crumbsConfig.cmake / crumbsTargets.cmake

option(CRUMBS_BUILD_IN_TREE "Add CRUMBS as a subdirectory and link the in-repo crumbs target" ON)
if(NOT DEFINED CRUMBS_PATH)
    # Default repository path is three levels up from the example dir
    set(CRUMBS_PATH ${CMAKE_SOURCE_DIR}/../../..)
endif()

if(CRUMBS_BUILD_IN_TREE)
    add_subdirectory(${CRUMBS_PATH} ${CMAKE_BINARY_DIR}/crumbs_subbuild)
    add_executable(crumbs_native_example main.c)
    target_link_libraries(crumbs_native_example PRIVATE crumbs)
    target_include_directories(crumbs_native_example PRIVATE ${CRUMBS_PATH}/src)
else()
    find_package(crumbs CONFIG REQUIRED)
    add_executable(crumbs_native_example main.c)
    target_link_libraries(crumbs_native_example PRIVATE crumbs::crumbs)
endif()
