#include "src/NES.h"

#include <iostream>

int main() {
    CPU cpu;
    PPU ppu;
    NES sys(&cpu, &ppu);
    Cartridge* c = sys.cart;

    if (c->open_file("smb.nes")) {
        std::ifstream& file = c->get_file();
        sys.run();
    }
}