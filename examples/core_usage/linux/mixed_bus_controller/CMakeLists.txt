cmake_minimum_required(VERSION 3.13)
project(crumbs_mixed_bus_controller C)

option(CRUMBS_BUILD_IN_TREE "Add CRUMBS as a subdirectory and link the in-repo crumbs target" ON)
if(NOT DEFINED CRUMBS_PATH)
    set(CRUMBS_PATH ${CMAKE_SOURCE_DIR}/../../../..)
endif()

if(CRUMBS_BUILD_IN_TREE)
    set(CRUMBS_ENABLE_LINUX_HAL ON CACHE BOOL "" FORCE)
    add_subdirectory(${CRUMBS_PATH} ${CMAKE_BINARY_DIR}/crumbs_subbuild)

    add_executable(crumbs_mixed_bus_controller main.c)
    target_link_libraries(crumbs_mixed_bus_controller PRIVATE crumbs)
    target_include_directories(crumbs_mixed_bus_controller PRIVATE ${CRUMBS_PATH}/src)
else()
    find_package(crumbs CONFIG REQUIRED)
    add_executable(crumbs_mixed_bus_controller main.c)
    target_link_libraries(crumbs_mixed_bus_controller PRIVATE crumbs::crumbs)
endif()

