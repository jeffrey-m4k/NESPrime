#include "NES.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"
#include "Display.h"
#include "IO.h"
#include "UI.h"
#include "APU/APU.h"
#include <SDL.h>
#include <SDL_ttf.h>

NES::NES() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0 || TTF_Init() != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    set_cpu(new CPU());
    set_ppu(new PPU());
    set_cart(new Cartridge());
    set_display(new Display());
    set_io(new IO());
    set_apu(new APU());
    set_ui(new UI());
    out.open("out.txt");
}

NES::~NES() {
    display->close();
}

void NES::run() {
    ui->init();

    while (!quit) {
        if (!ui->get_show()) tick(true, 1);

        Uint32 t = SDL_GetTicks();
        if (t - display->last_update >= 1000 / 60) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) quit = true;
                else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    SDL_Window *window = SDL_GetWindowFromID(event.window.windowID);
                    if (SDL_GetWindowID(window) == 3) quit = true;
                    else SDL_HideWindow(window);
                } else if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    if (ui->get_state() == PAUSE) ui->set_show(!ui->get_show());
                } else if (event.type == SDL_KEYDOWN) ui->get_show() ? ui->handle(event) : ui->handle_global(event);
            };

            if (!ui->get_show()) {
                ppu->output_pt();
                ppu->output_nt();

                display->refresh();
            } else {
                ui->tick();
                display->refresh();
            }

            display->last_update = t;
        }
    }
}

void NES::reset() {
    delete cart;
    delete cpu;
    delete ppu;
    delete apu;

    set_cart(new Cartridge());
    set_cpu(new CPU());
    set_ppu(new PPU());
    set_apu(new APU());

    clock = 0;
}

void NES::run(const std::string& filename) {
    if (cart->open_file(filename)) {
        this->filename = filename.substr(filename.find_last_of('/') + 1, filename.length());
        cart->load();
        cpu->init();
        ui->set_state(PAUSE);
        ui->set_show(false);
        SDL_Delay(250);
    }
}

void NES::tick(bool do_cpu, int times) {
    for (int i = 0; i < times; i++) {
        if (cpu->memory_regs[0x16] & 0x1) io->poll();
        if (clock % 12 == 0 && do_cpu) cpu->run();
        if (clock % 4 == 0) ppu->run();
        if (clock % 12 == 0) apu->cycle();

        clock++;
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

void NES::set_ui(UI* ui) {
    this->ui = ui;
    ui->set_nes(this);
}