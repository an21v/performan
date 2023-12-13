#! /bin/bash

set -x

clang++ -std=c++20 -Wall -pthread -I ../include/ sample.cpp -o run
