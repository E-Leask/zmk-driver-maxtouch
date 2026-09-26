cmake_minimum_required(VERSION 3.20)
project(test_input_maxtouch LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable CTest testing framework
include(CTest)
enable_testing()

# Fetch GoogleTest automatically via CMake FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# For MSVC runtime compatibility
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Define the test executable
add_executable(test_input_maxtouch
  test_input_maxtouch.cpp
  input_maxtouch_test_adapter.c
)

# Include directories
target_include_directories(test_input_maxtouch PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../drivers/input
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# Link Google Test & Google Mock
target_link_libraries(test_input_maxtouch PRIVATE
  GTest::gtest
  GTest::gmock
  GTest::gtest_main
)

# Discover tests and register them with CTest
include(GoogleTest)
gtest_discover_tests(test_input_maxtouch)
