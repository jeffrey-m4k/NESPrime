cmake_minimum_required(VERSION 3.24)
project(NESPrime)

set(CMAKE_CXX_STANDARD 17)

#set(SDL2_INCLUDE_DIR SDL2/include)
#set(SDL2_LIB_DIR SDL2/lib/x64)
#include_directories(${SDL2_INCLUDE_DIR})
#link_directories(${SDL2_LIB_DIR})

include_directories(${PROJECT_SOURCE_DIR}/include/SDL2)
link_directories(${PROJECT_SOURCE_DIR}/lib)

add_executable(NESPrime main.cpp src/Cartridge.cpp src/util.h src/Processor.cpp src/Memory.cpp src/Processor.cpp src/NES.cpp src/CPU.cpp src/PPU.cpp src/Bus.cpp src/Component.cpp src/Display.cpp src/mappers/Mapper.cpp src/mappers/Mapper0.cpp)

target_link_libraries(NESPrime SDL2main SDL2)
#set(GCC_COVERAGE_COMPILE_FLAGS "-ISDL2/include/SDL2")
#set(CMAKE_CXX_FLAGS ${GCC_COVERAGE_COMPILE_FLAGS})