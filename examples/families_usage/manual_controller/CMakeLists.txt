cmake_minimum_required(VERSION 3.13)
project(lhwit_manual_controller C)

# Option: build in-tree (as part of CRUMBS repo) or against installed CRUMBS
option(CRUMBS_BUILD_IN_TREE "Build as part of CRUMBS source tree" ON)

if(CRUMBS_BUILD_IN_TREE)
    # Building in-tree: CRUMBS source is 3 levels up
    # examples/families_usage/manual_controller -> examples/families_usage -> examples -> root
    set(CRUMBS_PATH "${CMAKE_SOURCE_DIR}/../../.." CACHE PATH "Path to CRUMBS root")
    
    # Force enable Linux HAL when building controllers
    set(CRUMBS_ENABLE_LINUX_HAL ON CACHE BOOL "Enable Linux I2C HAL" FORCE)
    
    # Add CRUMBS as subdirectory
    add_subdirectory(${CRUMBS_PATH} ${CMAKE_BINARY_DIR}/crumbs_build)
    
    # Target executable
    add_executable(lhwit_manual_controller main.c)
    
    # Include directories
    target_include_directories(lhwit_manual_controller PRIVATE
        ${CRUMBS_PATH}/src
        ${CMAKE_SOURCE_DIR}/..  # Parent dir for lhwit_family/ headers
    )
    
    # Link against CRUMBS (in-tree target)
    target_link_libraries(lhwit_manual_controller PRIVATE crumbs)
    
else()
    # Building against installed CRUMBS
    find_package(crumbs REQUIRED)
    
    # Target executable
    add_executable(lhwit_manual_controller main.c)
    
    # Include parent directory for lhwit_family headers
    target_include_directories(lhwit_manual_controller PRIVATE
        ${CMAKE_SOURCE_DIR}/..
    )
    
    # Link against installed CRUMBS
    target_link_libraries(lhwit_manual_controller PRIVATE crumbs::crumbs)
endif()

# Set C standard
set_target_properties(lhwit_manual_controller PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)
