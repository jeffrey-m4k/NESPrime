cmake_minimum_required(VERSION 3.24)
project(NESPrime)

set(CMAKE_CXX_STANDARD 17)

add_executable(NESPrime main.cpp src/Cartridge.cpp src/util.h src/Processor.cpp src/Memory.cpp src/Processor.cpp src/NES.cpp src/CPU.cpp src/PPU.cpp)
