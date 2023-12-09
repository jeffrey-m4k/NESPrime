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

// === MAPPER 0 ===

uint8_t* Mapper::map_cpu(uint16_t addr) {
    if (addr < 0x2000) return cpu_mem + (addr % 0x800);
    else if (addr >= 0x8000) {
        if (prg_size != 0x8000) addr &= ~0x4000;
        return prg_rom + (addr - 0x8000);
    } else if (addr >= 0x6000 && prg_ram != nullptr) {
        return prg_ram + (addr - 0x6000);
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
        case OneScreen_LB: return base + addr % 0x400;
        case OneScreen_HB: return base + addr % 0x400 + 0x800;
        default: return nullptr;
    }
}

// === MAPPER 1 ===

uint8_t* Mapper1::map_cpu(uint16_t address) {
    if (address < 0x8000) return Mapper::map_cpu(address);
    switch (bankmode_prg) {
        case 0:
        case 1:
            return prg_rom + (bank_prg & ~0x1) * 0x8000 + (address - 0x8000);
        case 2:
            if (address < 0xC000) return prg_rom + (address - 0x8000);
            else return prg_rom + bank_prg * 0x4000 + (address - 0xC000);
        case 3:
        default:
            if (address >= 0xC000) return prg_rom + (prg_size - 0x4000) + (address - 0xC000);
            else return prg_rom + bank_prg * 0x4000 + (address - 0x8000);
    }
}

uint8_t* Mapper1::map_ppu(uint16_t address) {
    if (address >= 0x2000) return Mapper::map_ppu(address);

    uint8_t* chr_mem = chr_ram == nullptr ? chr_rom : chr_ram;
    if (bankmode_chr) {
        if (address < 0x1000) return chr_mem + bank_chr * 0x1000 + address;
        else return chr_mem + bank_chr_2 * 0x1000 + (address - 0x1000);
    } else {
        return chr_mem + (bank_chr & ~0x1) * 0x2000 + address;
    }
}

void Mapper1::handle_write(uint8_t data, uint16_t addr) {
    long cyc = cartridge->get_nes()->get_cpu()->get_cycle();
    if (addr < 0x8000) return;
    if (data & 0x80) { shifter = 0x80; bankmode_prg = 3; last_write = cyc; return; } // clear the shift register if bit 7 is set

    if (cyc - last_write > 1) { // ignore consecutive writes
        shifter = (shifter >> 1) | ((data & 0x1) << 7); // shift right with bit 0 of data

        if (shifter & 0x4) { // if bit 2 is 1, the shifter is full and we update the relevant bank
            uint8_t shifter_out = (shifter & 0xF8) >> 3;

            if (addr >= 0xE000) { // PRG bank switch
                bank_prg = shifter_out & 0xF;
                if (!(bankmode_prg & 0x2)) bank_prg &= ~0x1; // if 32KB prg banking, ignore low bit

            } else if (addr >= 0xC000) { // CHR bank-2 switch
                bank_chr_2 = shifter_out;

            } else if (addr >= 0xA000) { // CHR bank-1 switch
                bank_chr = shifter_out;

            } else { // control
                switch(shifter_out & 0x3) {
                    case 0: set_mirroring(OneScreen_LB); break;
                    case 1: set_mirroring(OneScreen_HB); break;
                    case 2: set_mirroring(Vertical); break;
                    case 3: set_mirroring(Horizontal); break;
                }

                bankmode_prg = (shifter_out >> 2) & 0x3;
                bankmode_chr = (shifter_out >> 4) & 0x1;

            }

            shifter = 0x80;
        }
    }

    last_write = cyc;
}

