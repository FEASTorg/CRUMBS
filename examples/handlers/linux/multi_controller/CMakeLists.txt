cmake_minimum_required(VERSION 3.10)
project(multi_controller C)

set(CMAKE_C_STANDARD 99)

# CRUMBS source directory
set(CRUMBS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src")

# Include directories
include_directories(
    ${CRUMBS_SRC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../commands
)

# Source files
set(SOURCES
    multi_controller.c
    ${CRUMBS_SRC_DIR}/core/crumbs_core.c
    ${CRUMBS_SRC_DIR}/hal/linux/crumbs_i2c_linux.c
    ${CRUMBS_SRC_DIR}/crc/crumbs_crc8.c
)

# Build executable
add_executable(multi_controller ${SOURCES})

# Link math library if needed
target_link_libraries(multi_controller m)
