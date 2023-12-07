#ifndef NES_H
#define NES_H

#include <string>
#include <fstream>

class CPU;
class PPU;
class Cartridge;
class Bus;
class Display;
class IO;
class APU;
class UI;

class NES {
public:
    NES();
    ~NES();
    void run();
    void run(const std::string& filename);
    void tick(bool do_cpu, int times);

    CPU* get_cpu() { return cpu; };
    PPU* get_ppu() { return ppu; };
    Bus* get_bus() { return bus; };
    Cartridge* get_cart() { return cart; }
    Display* get_display() { return display; }
    IO* get_io() { return io; }
    APU* get_apu() { return apu; }
    UI* get_ui() { return ui; }
    void set_cpu(CPU* cpu);
    void set_ppu(PPU* ppu);
    void set_cart(Cartridge* cart);
    void set_display(Display* display);
    void set_io(IO* io);
    void set_apu(APU* apu);
    void set_ui(UI* ui);

    long get_clock() { return clock; };

    std::ofstream out;
    std::string filename;
private:
    Cartridge* cart;
    Bus* bus;
    CPU* cpu;
    PPU* ppu;
    Display* display;
    IO* io;
    APU* apu;
    UI* ui;

    long clock = 0;
    constexpr static const int CPS = 21477272;
    constexpr static const double CLOCK_SPEED = 1.0 / CPS;
    int cycles_delta = 0;
};


#endif //NES_H
