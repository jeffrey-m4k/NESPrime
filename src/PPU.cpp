
#include "PPU.h"

PPU::PPU() : Processor() {
    this->PPU::reset();
}

void PPU::reset() {
    this->aspace.clear();
    uint8_t* memory = this->mem.get_mem();
    switch (nt_mirror) {
        case Horizontal:
            for (int i = 0; i < 2; ++i) {
                map(0x2000+(i*0x400), memory, 0x400);
                map(0x2800+(i*0x400), memory + 0x400, 0x400);
                map(0x3000+(i*0x400), memory, 0x400);
                map(0x3800+(i*0x400), memory + 0x400, 0x400);
            }
            break;
        case Vertical:
            for (int i = 0; i < 2; ++i) {
                map(0x2000+(i*0x800), memory, 0x400);
                map(0x2400+(i*0x800), memory + 0x400, 0x400);
                map(0x3000+(i*0x800), memory, 0x400);
                map(0x3400+(i*0x800), memory + 0x400, 0x400);
            }
            break;
        default:
            break;
    }
    for (int i = 0; i < 8; ++i) {
        map(0x3F00+(i*0x20), palette, 0x20);
    }
}

void PPU::run() {}

uint16_t PPU::mirror_palette_addr(uint16_t addr) {
    if ((addr & 0x3F00) == 0x3F00 && (((addr >> 4) & 0xF)) % 2 != 0)
        addr -= 0x10;
    return addr;
}

uint8_t PPU::read_reg(PPU_REG reg_id, int cycle) {
    return regs[reg_id];
}

bool PPU::write_reg(PPU_REG reg_id, uint8_t value, int cycle) {
    if (cycle < 29658 && (reg_id == PPUCTRL || reg_id == PPUMASK || reg_id == PPUSCROLL || reg_id == PPUADDR))
        return false;
    else
        regs[reg_id] = value;
    return true;
}
