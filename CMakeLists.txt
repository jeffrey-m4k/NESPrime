cmake_minimum_required(VERSION 3.24)
project(NESPrime)

set(CMAKE_CXX_STANDARD 17)

add_executable(NESPrime main.cpp src/loader.cpp src/util.h)
