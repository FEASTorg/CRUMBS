cmake_minimum_required(VERSION 3.13)
project(lhwit_discovery_controller C)

# Build in-tree (inside CRUMBS repo)
option(CRUMBS_BUILD_IN_TREE "Add CRUMBS as a subdirectory" ON)
if(NOT DEFINED CRUMBS_PATH)
    # Repository path is three levels up from this example
    set(CRUMBS_PATH ${CMAKE_SOURCE_DIR}/../../..)
endif()

if(CRUMBS_BUILD_IN_TREE)
    # Enable Linux HAL for I2C support
    set(CRUMBS_ENABLE_LINUX_HAL ON CACHE BOOL "" FORCE)
    add_subdirectory(${CRUMBS_PATH} ${CMAKE_BINARY_DIR}/crumbs_subbuild)
    
    add_executable(lhwit_discovery_controller main.c)
    target_link_libraries(lhwit_discovery_controller PRIVATE crumbs)
    target_include_directories(lhwit_discovery_controller PRIVATE 
        ${CRUMBS_PATH}/src
        ${CMAKE_SOURCE_DIR}/..
    )
else()
    find_package(crumbs CONFIG REQUIRED)
    add_executable(lhwit_discovery_controller main.c)
    target_link_libraries(lhwit_discovery_controller PRIVATE crumbs::crumbs)
    target_include_directories(lhwit_discovery_controller PRIVATE 
        ${CMAKE_SOURCE_DIR}/..
    )
endif()
