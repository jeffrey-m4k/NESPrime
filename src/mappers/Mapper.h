#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include "../Cartridge.h"
#include "../PPU.h"
#include "../CPU.h"

enum MIRRORING {
    Horizontal, Vertical, OneScreen, FourScreen
};

class Mapper {
public:
    explicit Mapper(Cartridge* cart);
    ~Mapper() = default;
    virtual uint8_t* map_cpu(uint16_t addr);
    virtual uint8_t* map_ppu(uint16_t addr);
    void set_mirroring(MIRRORING mirr) { mirroring = mirr; }
    MIRRORING get_mirroring() { return mirroring; }
protected:
    Cartridge* cartridge;
    MIRRORING mirroring;
    uint8_t* cpu_mem;
    uint8_t* ppu_mem;
    uint8_t* prg_rom;
    uint8_t* chr_rom;
};

#endif //MAPPER_H
