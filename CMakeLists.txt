cmake_minimum_required(VERSION 3.20)
project(XtionProLiveDriver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

# Module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find required packages
find_package(PkgConfig)

# libusb
if(WIN32)
    # On Windows, we expect libusb to be found via vcpkg (include/libusb-1.0, libusb-1.0.lib)
    find_path(LIBUSB_INCLUDE_DIRS libusb.h PATH_SUFFIXES libusb-1.0)
    find_library(LIBUSB_LIBRARIES NAMES libusb-1.0 usb-1.0)
    if(NOT LIBUSB_INCLUDE_DIRS OR NOT LIBUSB_LIBRARIES)
        message(FATAL_ERROR "libusb not found. Install via vcpkg: vcpkg install libusb:x64-windows")
    endif()
else()
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
endif()

# OpenNI2 (optional, for integration)
set(OPENNI2_INCLUDE $ENV{OPENNI2_INCLUDE})
set(OPENNI2_REDIST $ENV{OPENNI2_REDIST})

if(OPENNI2_INCLUDE AND OPENNI2_REDIST)
    set(OPENNI2_FOUND TRUE)
    message(STATUS "OpenNI2 found: ${OPENNI2_INCLUDE}")
else()
    message(WARNING "OpenNI2 not found. Set OPENNI2_INCLUDE and OPENNI2_REDIST environment variables.")
    set(OPENNI2_FOUND FALSE)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBUSB_INCLUDE_DIRS}
)

if(OPENNI2_FOUND)
    include_directories(${OPENNI2_INCLUDE})
endif()

# Source files
set(DRIVER_SOURCES
    src/XtionDevice.cpp
    src/DepthStream.cpp
    src/ColorStream.cpp
    src/IRStream.cpp
    src/USBCommunication.cpp
    src/ProtocolHandler.cpp
)

set(DRIVER_HEADERS
    include/XtionDevice.h
    include/DepthStream.h
    include/ColorStream.h
    include/IRStream.h
    include/USBCommunication.h
    include/ProtocolHandler.h
    include/XtionCommon.h
)

# Main driver library
add_library(XtionDriver SHARED
    ${DRIVER_SOURCES}
    ${DRIVER_HEADERS}
)

target_link_libraries(XtionDriver
    PUBLIC
    ${LIBUSB_LIBRARIES}
)

# Export all symbols so tests/examples can link against the DLL on Windows
set_target_properties(XtionDriver PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Compiler warnings
if(MSVC)
    target_compile_options(XtionDriver PRIVATE /W4)
else()
    target_compile_options(XtionDriver PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation
if(OPENNI2_FOUND)
    install(TARGETS XtionDriver
        DESTINATION ${OPENNI2_REDIST}/OpenNI2/Drivers
    )
else()
    install(TARGETS XtionDriver
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

install(FILES ${DRIVER_HEADERS}
    DESTINATION include/XtionDriver
)

# Tests
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "========================================")
message(STATUS "Xtion Pro Live Driver Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "libusb: ${LIBUSB_LIBRARIES}")
message(STATUS "OpenNI2 found: ${OPENNI2_FOUND}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "========================================")
message(STATUS "")
