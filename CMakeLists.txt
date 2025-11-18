cmake_minimum_required(VERSION 3.15)

# Project configuration
project(JonssonicDSP 
    VERSION 0.1.0
    DESCRIPTION "A modular, approachable C++ audio DSP library"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(JONSSONIC_BUILD_TESTS "Build the tests" ON)
option(JONSSONIC_BUILD_EXAMPLES "Build example programs" ON)
option(JONSSONIC_BUILD_SHARED "Build shared library instead of static" OFF)

# Create library target
if(JONSSONIC_BUILD_SHARED)
    add_library(JonssonicDSP SHARED)
else()
    add_library(JonssonicDSP STATIC)
endif()

# Add an alias for consistent namespacing
add_library(Jonssonic::DSP ALIAS JonssonicDSP)

# Set library properties
set_target_properties(JonssonicDSP PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jonssonic-dsp"
    POSITION_INDEPENDENT_CODE ON
)

# Collect source files
file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE EFFECTS_SOURCES "src/effects/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/*.cpp")

# Collect header files
file(GLOB_RECURSE CORE_HEADERS "include/Jonssonic/core/*.hpp" "include/Jonssonic/core/*.h")
file(GLOB_RECURSE EFFECTS_HEADERS "include/Jonssonic/effects/*.hpp" "include/Jonssonic/effects/*.h")
file(GLOB_RECURSE UTILS_HEADERS "include/Jonssonic/utils/*.hpp" "include/Jonssonic/utils/*.h")

# Add sources to library
target_sources(JonssonicDSP
    PRIVATE
        ${CORE_SOURCES}
        ${EFFECTS_SOURCES}
        ${UTILS_SOURCES}
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES
            ${CORE_HEADERS}
            ${EFFECTS_HEADERS}
            ${UTILS_HEADERS}
)

# Include directories
target_include_directories(JonssonicDSP
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler warnings
if(MSVC)
    target_compile_options(JonssonicDSP PRIVATE /W4 /WX-)
else()
    target_compile_options(JonssonicDSP PRIVATE 
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )
endif()

# Platform-specific settings
if(APPLE)
    target_compile_definitions(JonssonicDSP PRIVATE JONSSONIC_MACOS)
elseif(WIN32)
    target_compile_definitions(JonssonicDSP PRIVATE JONSSONIC_WINDOWS)
elseif(UNIX)
    target_compile_definitions(JonssonicDSP PRIVATE JONSSONIC_LINUX)
endif()

# Link math library on Unix
if(UNIX AND NOT APPLE)
    target_link_libraries(JonssonicDSP PRIVATE m)
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
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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
message(STATUS "  Library Type: ${JONSSONIC_BUILD_SHARED}")
