#include "NES.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"
#include "Display.h"
#include "IO.h"
#include "APU/APU.h"
#include <SDL.h>

NES::NES() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    set_cpu(new CPU());
    set_ppu(new PPU());
    set_cart(new Cartridge());
    set_display(new Display());
    set_io(new IO());
    set_apu(new APU());
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
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) quit = true;
        };

        while (cycles_delta < cycles_per_frame) {
            tick(true, 1);
        }

        if (SDL_GetTicks() - display->last_update >= 1000/60) {
            cycles_delta -= cycles_per_frame;
            display->refresh();
            ppu->output_pt();
            ppu->output_nt();
        }
    }
}

void NES::run(const std::string& filename) {
    if (cart->open_file(filename)) {
        run();
    }
}

void NES::tick(bool do_cpu, int times) {
    for (int i = 0; i < times; i++) {
        if (clock % 487 == 0) {
            int16_t sample = apu->get_mixer() * 2500;
            const int sample_size = sizeof(int16_t) * 1;
            SDL_QueueAudio(apu->audio_device, &sample, sample_size);
        }
        if (cpu->get_memory_reg(0x16) & 0x1) io->poll();
        if (clock % 12 == 0 && do_cpu) cpu->run();
        if (clock % 12 == 0) apu->cycle();
        if (clock % 4 == 0) ppu->run();

        clock++;
        cycles_delta++;
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

void NES::set_io(IO* io) {
    this->io = io;
    io->set_nes(this);
}

void NES::set_apu(APU* apu) {
    this->apu = apu;
    apu->set_nes(this);
}