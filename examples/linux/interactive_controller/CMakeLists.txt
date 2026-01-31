cmake_minimum_required(VERSION 3.13)
project(crumbs_interactive_controller C)

# Build this example in one of two ways:
# 1) in-tree: build inside the CRUMBS repo and link to the `crumbs` target
#    cmake -S examples/linux/interactive_controller -B build \
#          -DCRUMBS_BUILD_IN_TREE=ON
# 2) out-of-tree (installed library): set CMAKE_PREFIX_PATH to an installed
#    path that contains crumbsConfig.cmake / crumbsTargets.cmake

option(CRUMBS_BUILD_IN_TREE "Add CRUMBS as a subdirectory and link the in-repo crumbs target" ON)
if(NOT DEFINED CRUMBS_PATH)
    # Default repository path is three levels up from the example dir
    set(CRUMBS_PATH ${CMAKE_SOURCE_DIR}/../../..)
endif()

if(CRUMBS_BUILD_IN_TREE)
    # Enable Linux HAL for I2C support, disable examples to avoid target conflicts
    set(CRUMBS_ENABLE_LINUX_HAL ON CACHE BOOL "" FORCE)
    set(CRUMBS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CRUMBS_PATH} ${CMAKE_BINARY_DIR}/crumbs_subbuild)
    
    add_executable(crumbs_interactive_controller interactive_controller.c)
    target_link_libraries(crumbs_interactive_controller PRIVATE crumbs)
    target_include_directories(crumbs_interactive_controller PRIVATE
        ${CRUMBS_PATH}/src
        ${CRUMBS_PATH}/examples/common
    )
else()
    find_package(crumbs CONFIG REQUIRED)
    add_executable(crumbs_interactive_controller interactive_controller.c)
    target_link_libraries(crumbs_interactive_controller PRIVATE crumbs::crumbs)
    # Command headers need to be copied or found separately for out-of-tree
endif()
