#include "NES.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"
#include "Display.h"

NES::NES() {
    set_cpu(new CPU());
    set_ppu(new PPU());
    set_cart(new Cartridge());
    set_display(new Display());
    out.open("out.txt");
}

NES::~NES() {
    display->close();
}

void NES::run() {
    cart->load();
    cpu->init();
    display->refresh();

    int cycles_per_frame = CPS/60;
    int cycles_delta = 0;

    while (clock < CPS*20) {
        while (cycles_delta < cycles_per_frame) {
            if (clock % 12 == 0) cpu->run();
            if (clock % 4 == 0) ppu->run();


            clock++;
            cycles_delta++;
        }
        cycles_delta -= cycles_per_frame;
        display->refresh();
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

void NES::set_display(Display* display) {
    this->display = display;
    display->set_nes(this);
}