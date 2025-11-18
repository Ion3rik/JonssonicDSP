# Tests CMakeLists.txt for JonssonicDSP

# Option to use system-installed GoogleTest or fetch it
option(JONSSONIC_USE_SYSTEM_GTEST "Use system-installed GoogleTest" OFF)

if(JONSSONIC_USE_SYSTEM_GTEST)
    find_package(GTest REQUIRED)
else()
    # Fetch GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Enable CMake's test framework
enable_testing()

# Collect test files
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# Create test executable
add_executable(JonssonicDSP_Tests ${TEST_SOURCES})

# Link against the library and GoogleTest
target_link_libraries(JonssonicDSP_Tests
    PRIVATE
        JonssonicDSP
        GTest::gtest_main
)

# Set C++ standard for tests (same as main library)
set_target_properties(JonssonicDSP_Tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(JonssonicDSP_Tests)

# Add test as a CTest target
add_test(NAME JonssonicDSP_AllTests COMMAND JonssonicDSP_Tests)
