cmake_minimum_required(VERSION 3.13)

project(CRUMBS
    VERSION 0.6.0
    LANGUAGES C
)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------

option(CRUMBS_ENABLE_LINUX_HAL "Enable Linux I2C HAL (requires linux-wire)" ON)
option(CRUMBS_BUILD_EXAMPLES   "Build Linux example programs"               ON)

# ---------------------------------------------------------------------------
# Global C settings
# ---------------------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

# Core protocol + CRC (platform-independent C)
set(CRUMBS_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/crumbs_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crc/crumbs_crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crc/c99/crc8_nibble.c
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
        ${CMAKE_CURRENT_SOURCE_DIR}/src          # crumbs.h, crumbs_*.h, crumbs_linux.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crc/c99
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/linux
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

# ---------------------------------------------------------------------------
# (Optional) Install rules – keep minimal for now
# ---------------------------------------------------------------------------

# Install the static library
install(TARGETS crumbs
    ARCHIVE DESTINATION lib
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
