cmake_minimum_required(VERSION 3.13)
project(crumbs_mock_linux_controller C)

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
    
    add_executable(crumbs_mock_linux_controller mock_controller.c)
    target_link_libraries(crumbs_mock_linux_controller PRIVATE crumbs)
    target_include_directories(crumbs_mock_linux_controller PRIVATE 
        ${CRUMBS_PATH}/src
        ${CMAKE_SOURCE_DIR}/..  # Parent directory for mock_ops.h
    )
else()
    find_package(crumbs CONFIG REQUIRED)
    add_executable(crumbs_mock_linux_controller mock_controller.c)
    target_link_libraries(crumbs_mock_linux_controller PRIVATE crumbs::crumbs)
    target_include_directories(crumbs_mock_linux_controller PRIVATE ${CMAKE_SOURCE_DIR}/..)
endif()
