cmake_minimum_required(VERSION 3.14)

set(SOURCE_FILES sample.cpp)

add_executable(sample ${SOURCE_FILES})
target_link_libraries(sample performan)

target_include_directories(sample PUBLIC ${PROJECT_SOURCE_DIR}/include)
