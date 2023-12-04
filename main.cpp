#include "src/NES.h"

#define SDL_MAIN_HANDLED

#include <iostream>

int main() {
    NES* nes = new NES();

    nes->run("roms/test/color_test.nes");
}