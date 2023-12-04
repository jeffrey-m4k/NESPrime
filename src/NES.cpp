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
    cycles_delta = 0;

    bool quit = false;
    while (!quit) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = true;
        }

        while (cycles_delta < cycles_per_frame) {
            tick(true, 1);
        }
//        if (ppu->get_y() == 240 && ppu->get_x() == )
        //if (SDL_GetTicks() - display->last_update >= 1000/60) {
            cycles_delta -= cycles_per_frame;
            display->refresh();
        //}
//        SDL_Delay(1000/60);
    }
}

void NES::run(const std::string& filename) {
    if (cart->open_file(filename)) {
        run();
    }
}

void NES::tick(bool do_cpu, int times) {
    for (int i = 0; i < times; i++) {
        if (clock % 12 == 0 && do_cpu) cpu->run();
        if (clock % 4 == 0) ppu->run();

        clock++;
        cycles_delta++;

//        if (cycles_delta >= CPS/60*4) {
//            cycles_delta -= CPS/60*4;
//            display->refresh();
//        }
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