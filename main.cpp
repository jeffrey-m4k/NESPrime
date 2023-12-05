#include "src/NES.h"

#define SDL_MAIN_HANDLED

#include <iostream>

int main() {
    NES* nes = new NES();

    nes->run("roms/mapper2/Castlevania.nes");
}