#ifndef NES_H
#define NES_H

#include <string>
#include <fstream>

class CPU;
class PPU;
class Cartridge;
class Bus;

class NES {
public:
    NES();
    ~NES() {};
    void run();
    void run(const std::string& filename);

    CPU* get_cpu() { return cpu; };
    PPU* get_ppu() { return ppu; };
    Bus* get_bus() { return bus; };
    Cartridge* get_cart() { return cart; }
    void set_cpu(CPU* cpu);
    void set_ppu(PPU* ppu);
    void set_cart(Cartridge* cart);

    long get_clock() { return clock; };

    std::ofstream out;
private:
    Cartridge* cart;
    Bus* bus;
    CPU* cpu;
    PPU* ppu;

    long clock = 0;
};


#endif //NES_H
