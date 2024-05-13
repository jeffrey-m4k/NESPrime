#include "NES.h"

#include <iostream>
#include "SDL.h"
#include <nfd.h>

int main(int argc, char *argv[]) {
    NFD_Init();

    NES* nes = new NES();
    nes->run();

    NFD_Quit();
    return EXIT_SUCCESS;
}