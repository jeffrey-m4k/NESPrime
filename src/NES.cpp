#include "NES.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"

NES::NES() {
    set_cpu(new CPU());
    set_ppu(new PPU());
    set_cart(new Cartridge());
    out.open("out.txt");
}

void NES::run() {
    cart->load();
    cpu->init();

    while (clock < 1000000) {
        if (clock % 12 == 0) cpu->run();
        if (clock % 4 == 0) ppu->run();

        clock++;
    }
}

void NES::run(const std::string& filename) {
    if (cart->open_file(filename)) {
        run();
    }
}

void NES::set_cpu(CPU* cpu) {
    this->cpu = cpu;
    cpu->set_nes(this);
}

void NES::set_ppu(PPU* ppu) {
    this->ppu = ppu;
    ppu->set_nes(this);
}

void NES::set_cart(Cartridge* cart) {
    this->cart = cart;
    cart->set_nes(this);
}