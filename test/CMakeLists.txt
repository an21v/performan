cmake_minimum_required(VERSION 3.14)

enable_testing()

add_executable(performantest main.cpp)
target_link_libraries(performantest GTest::gtest_main performan)
target_include_directories(performantest PRIVATE ${PROJECT_SOURCE_DIR}/include)
include(GoogleTest)
gtest_discover_tests(performantest)
