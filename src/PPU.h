#ifndef PPU_H
#define PPU_H

#include "Processor.h"

enum MIRRORING {
    Horizontal, Vertical, OneScreen, FourScreen
};

enum PPU_REG {
    PPUCTRL, PPUMASK, PPUSTATUS, OAMADDR, OAMDATA, PPUSCROLL, PPUADDR, PPUDATA, OAMDMA
};

class PPU : public Processor {
public:
    PPU();
    ~PPU() {};
    virtual void reset() override;
    virtual void init() override;
    virtual bool run() override;
    void set_mirroring(MIRRORING mirror_mode) { nt_mirror = mirror_mode; }
    uint8_t read_reg(uint8_t reg_id, int cycle);
    bool write_reg(uint8_t reg_id, uint8_t value, int cycle);
protected:
    uint8_t read(int addr) override { return Processor::read(mirror_palette_addr(addr)); };
    bool write(const uint16_t addr, const uint8_t data) override { return Processor::write(mirror_palette_addr(addr), data); };
private:
    static uint16_t mirror_palette_addr(uint16_t addr);
    MIRRORING nt_mirror;
    uint8_t oam[256];
    uint8_t palette[32];
    uint8_t regs[9];
};


#endif //PPU_H
