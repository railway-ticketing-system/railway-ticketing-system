cmake_minimum_required(VERSION 3.20)
project(rtrs VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RTRS_COVERAGE "Build with gcov coverage instrumentation" OFF)
option(RTRS_BUILD_TESTS "Build unit tests" ON)

include(FetchContent)
FetchContent_Declare(httplib   URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.3.tar.gz)
FetchContent_Declare(json      URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_Declare(sqlite3   URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip)
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz)
set(HTTPLIB_COMPILE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(httplib json sqlite3)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# SQLite amalgamation has no CMake project, so build it as a small static lib.
add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
set_target_properties(sqlite3 PROPERTIES C_STANDARD 11)

file(GLOB_RECURSE RTRS_SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(FILTER RTRS_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(rtrs_core STATIC ${RTRS_SOURCES})
target_include_directories(rtrs_core PUBLIC src)
target_link_libraries(rtrs_core PUBLIC sqlite3 nlohmann_json::nlohmann_json
                      httplib::httplib OpenSSL::Crypto Threads::Threads ${CMAKE_DL_LIBS})
target_compile_options(rtrs_core PRIVATE -Wall -Wextra -Wpedantic)

if(RTRS_COVERAGE)
  target_compile_options(rtrs_core PUBLIC --coverage -O0 -g)
  target_link_options(rtrs_core PUBLIC --coverage)
endif()

add_executable(rtrs_server src/main.cpp)
target_link_libraries(rtrs_server PRIVATE rtrs_core)

if(RTRS_BUILD_TESTS)
  FetchContent_MakeAvailable(googletest)
  enable_testing()
  file(GLOB_RECURSE RTRS_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
  add_executable(rtrs_tests ${RTRS_TEST_SOURCES})
  target_link_libraries(rtrs_tests PRIVATE rtrs_core GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(rtrs_tests)
endif()
