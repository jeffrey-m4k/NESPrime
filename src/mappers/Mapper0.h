
#ifndef MAPPER0_H
#define MAPPER0_H

#include "Mapper.h"

class Mapper0 : public Mapper {
public:
    explicit Mapper0(Cartridge* cart) : Mapper(cart) { nrom_256 = cart->get_prg_size() == 0x8000; }
    uint8_t* map_cpu(uint16_t address) override;
    uint8_t* map_ppu(uint16_t address) override;
private:
    bool nrom_256;
};


#endif //MAPPER0_H
