cmake_minimum_required(VERSION 3.24)
project(NESPrime)

set(CMAKE_CXX_STANDARD 23)

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE)
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp src/Cartridge.cpp src/util.h src/Processor.cpp src/Memory.cpp src/Processor.cpp src/NES.cpp src/CPU.cpp src/PPU.cpp src/Component.cpp src/Display.cpp src/IO.cpp src/Mapper.cpp src/UI.cpp src/APU/APU.cpp src/APU/Units.cpp src/APU/Channel.cpp app.rc src/APU/SC_2A03.cpp src/APU/SC_5B.cpp src/APU/SoundChip.cpp)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(unofficial-nativefiledialog CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:unofficial::nativefiledialog::nfd>,unofficial::nativefiledialog::nfd,unofficial::nativefiledialog::nfd-static>)