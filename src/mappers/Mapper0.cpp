#include "Mapper0.h"

uint8_t* Mapper0::map_cpu(uint16_t address) {
    if (address < 0x2000) return Mapper::map_cpu(address);
    if (address < 0x8000) return nullptr;
    if (!nrom_256) address &= ~0x4000;
    address -= 0x8000;
    return prg_rom + address;
}

uint8_t* Mapper0::map_ppu(uint16_t address) {
    if (address < 0x2000) return chr_rom + address;
    else return Mapper::map_ppu(address);
}