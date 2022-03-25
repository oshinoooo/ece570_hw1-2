cmake_minimum_required(VERSION 3.19)

project(test)

set(CMAKE_CXX_STANDARD 14)

add_executable(test src/thread.cc tests/test7.cc)