#include "NES.h"

#include <iostream>
#include "SDL.h"

int main(int argc, char *argv[]) {
    NES* nes = new NES();

    nes->run();

    return EXIT_SUCCESS;
}