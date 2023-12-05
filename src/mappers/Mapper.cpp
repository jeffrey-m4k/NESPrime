#include "Mapper.h"

Mapper::Mapper(Cartridge *cart) : cartridge(cart) {
    cpu_mem = cartridge->get_nes()->get_cpu()->get_mem()->get_mem();
    ppu_mem = cartridge->get_nes()->get_ppu()->get_mem()->get_mem();
    prg_rom = cartridge->get_prg()->get_mem();
    chr_rom = cartridge->get_chr()->get_mem();
}

uint8_t* Mapper::map_cpu(uint16_t addr) {
    if (addr < 0x2000) return cpu_mem + (addr % 0x800);
    else return nullptr;
}

uint8_t* Mapper::map_ppu(uint16_t addr) {
    if (addr < 0x2000 || addr >= 0x3F00) return nullptr;
    addr &= ~0x1000; // $3000-$3FFF is a mirror of $2000-$2FFF
    addr -= 0x2000;
    uint8_t* base = ppu_mem;
    switch (mirroring) {
        case Horizontal: return base + (addr & ~0x400) % 0x800 + 0x400 * (addr / 0x800);
        case Vertical: return base + (addr & ~0x800);
        default: return nullptr;
    }
}


