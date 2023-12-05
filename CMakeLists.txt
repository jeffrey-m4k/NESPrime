cmake_minimum_required(VERSION 3.24)
project(NESPrime)

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/include/SDL2)
link_directories(${PROJECT_SOURCE_DIR}/lib)

add_executable(NESPrime main.cpp src/Cartridge.cpp src/util.h src/Processor.cpp src/Memory.cpp src/Processor.cpp src/NES.cpp src/CPU.cpp src/PPU.cpp src/Bus.cpp src/Component.cpp src/Display.cpp src/IO.cpp src/mappers/Mapper.cpp)

target_link_libraries(NESPrime SDL2main SDL2)