#ifndef NES_H
#define NES_H


#include <memory>
#include "Processor.h"
#include "Cartridge.h"

class Cartridge;
class CPU;
class PPU;

class NES {
public:
    //NES(CPU* cpu, PPU* ppu) : cpu(cpu), ppu(ppu) {};
    NES(CPU* cpu);
    ~NES() {};
    void run();
    CPU* get_cpu() { return cpu; };
    //PPU* get_ppu() { return ppu; };
public:
    Cartridge* cart;
private:
    CPU* cpu;
    //PPU* ppu;
};


#endif //NES_H
