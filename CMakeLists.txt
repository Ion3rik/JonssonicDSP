cmake_minimum_required(VERSION 3.15)

# Project configuration
project(JonssonicDSP 
    VERSION 0.1.0
	DESCRIPTION "JonssonicDSP - A Modular Realtime C++ Audio DSP Library"
	LANGUAGES CXX
)

# Global config options for header generation
set(JONSSONIC_MAX_CHANNELS 64)
set(JONSSONIC_MIN_SAMPLE_RATE 8000)
set(JONSSONIC_MAX_SAMPLE_RATE 192000)

# Generate jonssonic_config.h from template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/jonssonic/jonssonic_config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/jonssonic/jonssonic_config.h
    @ONLY
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(JONSSONIC_BUILD_TESTS "Build the tests" ON)
option(JONSSONIC_BUILD_EXAMPLES "Build example programs" ON)

# Create header-only library target
add_library(JonssonicDSP INTERFACE)

# Add an alias for consistent namespacing
add_library(Jonssonic::DSP ALIAS JonssonicDSP)

# Include directories
target_include_directories(JonssonicDSP
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Platform-specific compile definitions
if(APPLE)
    target_compile_definitions(JonssonicDSP INTERFACE JONSSONIC_MACOS)
elseif(WIN32)
    target_compile_definitions(JonssonicDSP INTERFACE JONSSONIC_WINDOWS)
elseif(UNIX)
    target_compile_definitions(JonssonicDSP INTERFACE JONSSONIC_LINUX)
endif()

# Link math library on Unix
if(UNIX AND NOT APPLE)
    target_link_libraries(JonssonicDSP INTERFACE m)
endif()

# Testing
if(JONSSONIC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(JONSSONIC_BUILD_EXAMPLES AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    add_subdirectory(examples)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS JonssonicDSP
    EXPORT JonssonicDSPTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install header files
install(DIRECTORY include/Jonssonic
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "detail" EXCLUDE
)

# Install the export set
install(EXPORT JonssonicDSPTargets
    FILE JonssonicDSPTargets.cmake
    NAMESPACE Jonssonic::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JonssonicDSP
)

# Create and install package configuration files
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/JonssonicDSPConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/JonssonicDSPConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JonssonicDSP
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/JonssonicDSPConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/JonssonicDSPConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/JonssonicDSPConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JonssonicDSP
)

# Print configuration summary
message(STATUS "JonssonicDSP Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests: ${JONSSONIC_BUILD_TESTS}")
message(STATUS "  Build Examples: ${JONSSONIC_BUILD_EXAMPLES}")
message(STATUS "  Library Type: INTERFACE (header-only)")
