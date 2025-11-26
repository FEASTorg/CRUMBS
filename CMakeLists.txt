cmake_minimum_required(VERSION 3.13)

project(CRUMBS
    VERSION 0.6.1
    LANGUAGES C
)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------

option(CRUMBS_ENABLE_LINUX_HAL "Enable Linux I2C HAL (requires linux-wire)" OFF)
option(CRUMBS_BUILD_EXAMPLES   "Build Linux example programs"               ON)
option(CRUMBS_ENABLE_TESTS "Build and run the lightweight C tests" ON)

# ---------------------------------------------------------------------------
# Global C settings
# ---------------------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Use GNUInstallDirs so we have CMAKE_INSTALL_LIBDIR and sane install locations
include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

# Core protocol + CRC (platform-independent C)
set(CRUMBS_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/crumbs_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crc/crumbs_crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crc/crc8_nibble.c
)

# Optionally add the Linux HAL implementation
if(CRUMBS_ENABLE_LINUX_HAL)
    list(APPEND CRUMBS_CORE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/linux/crumbs_i2c_linux.c
    )
endif()

# ---------------------------------------------------------------------------
# Library target
# ---------------------------------------------------------------------------

add_library(crumbs STATIC ${CRUMBS_CORE_SOURCES})

# Public headers live in src/ (for now)
target_include_directories(crumbs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/core>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/crc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/crc/c99>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/hal/linux>
        $<INSTALL_INTERFACE:include/crumbs>
)

# ---------------------------------------------------------------------------
# Link linux-wire if Linux HAL is enabled
# ---------------------------------------------------------------------------

if(CRUMBS_ENABLE_LINUX_HAL)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(WARNING "CRUMBS_ENABLE_LINUX_HAL=ON but system is not Linux; build may fail.")
    endif()

    # Expect linux-wire to provide a config package with target linux_wire::linux_wire
    find_package(linux_wire CONFIG REQUIRED)

    target_link_libraries(crumbs
        PUBLIC
            linux_wire::linux_wire
    )
endif()

# ---------------------------------------------------------------------------
# Examples (Linux controller)
# ---------------------------------------------------------------------------

if(CRUMBS_BUILD_EXAMPLES AND CRUMBS_ENABLE_LINUX_HAL)
    # simple_linux_controller example
    add_executable(crumbs_simple_linux_controller
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/linux/simple_controller/simple_linux_controller.c
    )

    target_link_libraries(crumbs_simple_linux_controller
        PRIVATE
            crumbs
    )

    target_include_directories(crumbs_simple_linux_controller
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
endif()

if(CRUMBS_ENABLE_TESTS)
    # Add a tiny test suite that does not require linux-wire or hardware.
    enable_testing()

    add_executable(test_encode_decode
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_encode_decode.c
    )

    target_link_libraries(test_encode_decode
        PRIVATE crumbs
    )

    add_executable(test_scan_fake
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_scan_fake.c
    )

    target_link_libraries(test_scan_fake
        PRIVATE crumbs
    )

    add_test(NAME encode_decode_test COMMAND test_encode_decode)
    add_executable(test_crc_failure
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_crc_failure.c
    )

    target_link_libraries(test_crc_failure
        PRIVATE crumbs
    )
    add_test(NAME scan_fake_test COMMAND test_scan_fake)
    add_test(NAME crc_failure_test COMMAND test_crc_failure)
endif()

# ---------------------------------------------------------------------------
# (Optional) Install rules – keep minimal for now
# ---------------------------------------------------------------------------

# Install the static library
# Install the static library and export targets so consumers can use find_package(crumbs)
install(TARGETS crumbs
    EXPORT crumbsTargets
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include/crumbs
)

# Install public headers under include/crumbs/
install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crumbs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crumbs_arduino.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crumbs_crc.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crumbs_i2c.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crumbs_message.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crumbs_linux.h
    DESTINATION include/crumbs
)

# ---------------------------------------------------------------------------
# Export / Config packaging for CMake consumers
# ---------------------------------------------------------------------------
include(CMakePackageConfigHelpers)

set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/crumbs")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/crumbsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/crumbsConfig.cmake"
    INSTALL_DESTINATION ${config_install_dir}
    # pass the CRUMBS_PACKAGE_DEPENDENCIES variable for substitution
    NO_CHECK_REQUIRED_COMPONENTS
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/crumbsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# If Linux HAL was enabled at build time, record a find_dependency snippet so
# installed crumbsConfig.cmake will require linux_wire when a consumer uses
# find_package(crumbs CONFIG).
if(CRUMBS_ENABLE_LINUX_HAL)
    set(CRUMBS_PACKAGE_DEPENDENCIES "find_dependency(linux_wire CONFIG REQUIRED)")
else()
    set(CRUMBS_PACKAGE_DEPENDENCIES "# no extra package dependencies")
endif()

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
