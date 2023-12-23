cmake_minimum_required(VERSION 3.8)

set(SOURCE_FILES performan.cpp ${PROJECT_SOURCE_DIR}/include/performan.h)

add_library(performan SHARED STATIC ${SOURCE_FILES})
