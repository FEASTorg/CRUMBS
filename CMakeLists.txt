cmake_minimum_required(VERSION 3.13)

project(CRUMBS
    VERSION 0.9.1
    LANGUAGES C
)

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------

option(CRUMBS_ENABLE_LINUX_HAL "Enable Linux I2C HAL (requires linux-wire)" OFF)
option(CRUMBS_BUILD_EXAMPLES   "Build Linux example programs"               ON)
option(CRUMBS_ENABLE_TESTS     "Build and run the lightweight C tests"       ON)

# -----------------------------------------------------------------------------
# Global C settings
# -----------------------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

set(CRUMBS_CORE_SOURCES
    src/core/crumbs_core.c
    src/crc/crumbs_crc.c
    src/crc/crc8_nibble.c
)

if(CRUMBS_ENABLE_LINUX_HAL)
    list(APPEND CRUMBS_CORE_SOURCES
        src/hal/linux/crumbs_i2c_linux.c
    )
endif()

# -----------------------------------------------------------------------------
# Library target
# -----------------------------------------------------------------------------

add_library(crumbs STATIC ${CRUMBS_CORE_SOURCES})

target_include_directories(crumbs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/crumbs>
)

# -----------------------------------------------------------------------------
# Linux HAL dependency
# -----------------------------------------------------------------------------

if(CRUMBS_ENABLE_LINUX_HAL)
    find_package(linux_wire CONFIG REQUIRED)
    target_link_libraries(crumbs PUBLIC linux_wire::linux_wire)
endif()

# -----------------------------------------------------------------------------
# Examples (Linux only)
# -----------------------------------------------------------------------------

if(CRUMBS_BUILD_EXAMPLES AND CRUMBS_ENABLE_LINUX_HAL)
    add_executable(crumbs_simple_linux_controller
        examples/linux/simple_controller/simple_linux_controller.c
    )
    target_link_libraries(crumbs_simple_linux_controller PRIVATE crumbs)

    add_executable(crumbs_handler_controller
        examples/linux/handler_controller/handler_controller.c
    )
    target_link_libraries(crumbs_handler_controller PRIVATE crumbs)
endif()

# -----------------------------------------------------------------------------
# Tests (hardware-independent)
# -----------------------------------------------------------------------------

if(CRUMBS_ENABLE_TESTS)
    enable_testing()

    add_executable(test_encode_decode tests/test_encode_decode.c)
    target_link_libraries(test_encode_decode PRIVATE crumbs)
    add_test(NAME encode_decode_test COMMAND test_encode_decode)

    add_executable(test_scan_fake tests/test_scan_fake.c)
    target_link_libraries(test_scan_fake PRIVATE crumbs)
    add_test(NAME scan_fake_test COMMAND test_scan_fake)

    add_executable(test_crc_failure tests/test_crc_failure.c)
    target_link_libraries(test_crc_failure PRIVATE crumbs)
    add_test(NAME crc_failure_test COMMAND test_crc_failure)

    add_executable(test_handlers tests/test_handlers.c)
    target_link_libraries(test_handlers PRIVATE crumbs)
    add_test(NAME handlers_test COMMAND test_handlers)
endif()

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------

install(TARGETS crumbs
    EXPORT crumbsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION include/crumbs
)

install(FILES
    src/crumbs.h
    src/crumbs_arduino.h
    src/crumbs_crc.h
    src/crumbs_i2c.h
    src/crumbs_message.h
    src/crumbs_linux.h
    DESTINATION include/crumbs
)

# -----------------------------------------------------------------------------
# Package Config Export
# -----------------------------------------------------------------------------

set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/crumbs")

if(CRUMBS_ENABLE_LINUX_HAL)
    set(CRUMBS_PACKAGE_DEPENDENCIES "find_dependency(linux_wire CONFIG REQUIRED)")
else()
    set(CRUMBS_PACKAGE_DEPENDENCIES "# no extra dependencies")
endif()

configure_package_config_file(
    cmake/crumbsConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/crumbsConfig.cmake"
    INSTALL_DESTINATION ${config_install_dir}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/crumbsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT crumbsTargets
    FILE crumbsTargets.cmake
    NAMESPACE crumbs::
    DESTINATION ${config_install_dir}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/crumbsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/crumbsConfigVersion.cmake"
    DESTINATION ${config_install_dir}
)
