#include "src/NES.h"

#include <iostream>

int main() {
    CPU cpu;
    //PPU ppu;
    NES sys(&cpu);
    Cartridge* c = sys.cart;

    if (c->open_file("nestest.nes")) {
        std::ifstream& file = c->get_file();
        sys.run();
    }
}