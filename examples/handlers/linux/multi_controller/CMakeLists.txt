cmake_minimum_required(VERSION 3.13)
project(crumbs_multi_controller C)

# Build this example in one of two ways:
# 1) in-tree: build inside the CRUMBS repo and link to the `crumbs` target
#    cmake -S examples/handlers/linux/multi_controller -B build \
#          -DCRUMBS_BUILD_IN_TREE=ON
# 2) out-of-tree (installed library): set CMAKE_PREFIX_PATH to an installed
#    path that contains crumbsConfig.cmake / crumbsTargets.cmake

option(CRUMBS_BUILD_IN_TREE "Add CRUMBS as a subdirectory and link the in-repo crumbs target" ON)
if(NOT DEFINED CRUMBS_PATH)
    # Default repository path is four levels up from the example dir
    set(CRUMBS_PATH ${CMAKE_SOURCE_DIR}/../../../..)
endif()

if(CRUMBS_BUILD_IN_TREE)
    # Enable Linux HAL for I2C support
    set(CRUMBS_ENABLE_LINUX_HAL ON CACHE BOOL "" FORCE)
    add_subdirectory(${CRUMBS_PATH} ${CMAKE_BINARY_DIR}/crumbs_subbuild)
    
    add_executable(crumbs_multi_controller multi_controller.c)
    target_link_libraries(crumbs_multi_controller PRIVATE crumbs)
    target_include_directories(crumbs_multi_controller PRIVATE
        ${CRUMBS_PATH}/src
        ${CRUMBS_PATH}/examples/commands
    )
else()
    find_package(crumbs CONFIG REQUIRED)
    add_executable(crumbs_multi_controller multi_controller.c)
    target_link_libraries(crumbs_multi_controller PRIVATE crumbs::crumbs)
    # Command headers need to be copied or found separately for out-of-tree
endif()
