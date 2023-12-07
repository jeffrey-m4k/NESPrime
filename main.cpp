#include "src/NES.h"

#define SDL_MAIN_HANDLED

#include <iostream>
#include "SDL.h"

int main(int argc, char *argv[]) {
    NES* nes = new NES();

    nes->run();
}