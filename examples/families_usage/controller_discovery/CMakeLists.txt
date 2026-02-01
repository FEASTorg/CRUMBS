cmake_minimum_required(VERSION 3.10)

project(lhwit_discovery_controller C)

# CRUMBS core
set(CRUMBS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
include_directories(
    ${CRUMBS_ROOT}/src
    ${CRUMBS_ROOT}/src/core
    ${CRUMBS_ROOT}/src/crc
    ${CRUMBS_ROOT}/src/hal/linux
)

add_executable(controller_discovery
    main.c
    ${CRUMBS_ROOT}/src/core/crumbs_core.c
    ${CRUMBS_ROOT}/src/crc/crumbs_crc.c
    ${CRUMBS_ROOT}/src/hal/linux/crumbs_linux.c
)

target_link_libraries(controller_discovery i2c)

install(TARGETS controller_discovery DESTINATION bin)
