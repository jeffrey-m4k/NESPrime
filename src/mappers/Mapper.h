#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include "../Cartridge.h"
#include "../PPU.h"
#include "../CPU.h"

class Mapper {
public:
    explicit Mapper(Cartridge* cart) : cartridge(cart) { mirroring = Vertical; };
    ~Mapper();
    virtual uint8_t* map_cpu(uint16_t addr);
    virtual uint8_t* map_ppu(uint16_t addr);
    void set_mirroring(MIRRORING mirr) { mirroring = mirr; }
protected:
    Cartridge* cartridge;
    MIRRORING mirroring;
};

#endif //MAPPER_H
