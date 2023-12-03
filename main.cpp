#include "src/NES.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <iostream>

int main() {
    NES* nes = new NES();

    nes->run("donkey kong.nes");
}