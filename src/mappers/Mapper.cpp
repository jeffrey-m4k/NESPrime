#include "Mapper.h"

Mapper::Mapper(Cartridge *cart) : cartridge(cart) {
    prg_size = cartridge->get_prg_size();
    chr_size = cartridge->get_chr_size();
    cpu_mem = cartridge->get_nes()->get_cpu()->get_mem()->get_mem();
    ppu_mem = cartridge->get_nes()->get_ppu()->get_mem()->get_mem();
    prg_rom = cartridge->get_prg_rom()->get_mem();
    chr_rom = cartridge->get_chr_rom()->get_mem();
    prg_ram = cartridge->get_prg_ram()->get_mem();
    chr_ram = cartridge->get_chr_ram()->get_mem();
    mirroring = Horizontal;
}

uint8_t* Mapper::map_cpu(uint16_t addr) {
    if (addr < 0x2000) return cpu_mem + (addr % 0x800);
    else if (addr >= 0x8000) {
        if (prg_size != 0x8000) addr &= ~0x4000;
        return prg_rom + (addr - 0x8000);
    }
    else return nullptr;
}

uint8_t* Mapper::map_ppu(uint16_t addr) {
    if (addr >= 0x3F00) return nullptr;
    if (addr < 0x2000) return chr_ram == nullptr ? chr_rom + addr : chr_ram + addr;
    addr &= ~0x1000; // $3000-$3FFF is a mirror of $2000-$2FFF
    addr -= 0x2000;
    uint8_t* base = ppu_mem;
    switch (mirroring) {
        case Horizontal: return base + (addr & ~0x400) % 0x800 + 0x400 * (addr / 0x800);
        case Vertical: return base + (addr & ~0x800);
        default: return nullptr;
    }
}


