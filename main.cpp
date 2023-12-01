#include "src/NES.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <iostream>

int main() {
    NES* nes = new NES();
    Cartridge* c = nes->get_cart();

    /*if (c->open_file("dk.nes")) {
        std::ifstream& file = c->get_file();
        nes->run();
    }*/
    nes->run("dk.nes");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("blah", SDL_GetError());
        return 1;
    }
}