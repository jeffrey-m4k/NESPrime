#ifndef PPU_H
#define PPU_H

#include "Processor.h"

enum MIRRORING {
    Horizontal, Vertical, OneScreen, FourScreen
};

enum PPU_REG {
    PPUCTRL, PPUMASK, PPUSTATUS, OAMADDR, OAMDATA, PPUSCROLL, PPUADDR, PPUDATA, OAMDMA
};

typedef uint8_t* Sprite;
enum SPRITE {Y, TILE, ATTR, X};

typedef uint8_t Tile[8][2];

class PPU : public Processor {
public:
    PPU();
    ~PPU() {};
    virtual void reset() override;
    virtual void init() override;
    virtual bool run() override;
    void set_mirroring(MIRRORING mirror_mode) { nt_mirror = mirror_mode; }
    uint8_t read_reg(uint8_t reg_id, int cycle);
    uint8_t read_reg(uint8_t reg_id, int cycle, bool physical_read);
    bool write_reg(uint8_t reg_id, uint8_t value, int cycle);
    void write_oam(uint8_t byte, uint8_t data);

    void set_palette(std::string palFileName);
    short get_x() { return scan_cycle; }
    short get_y() { return scanline; }

    uint16_t get_v() { return v; }
    void reset_address() { v = 0; address_latch_state = 0; }
    short get_latch_state() { return address_latch_state; }
protected:
    uint8_t read(int addr) override { return Processor::read(mirror_palette_addr(addr)); };
    bool write(const uint16_t addr, const uint8_t data) override { return Processor::write(mirror_palette_addr(addr), data); };
private:
    static uint16_t mirror_palette_addr(uint16_t addr);
    MIRRORING nt_mirror;
    uint8_t oam[256];
    uint8_t oam2[32];
    uint8_t palette[32];
    uint8_t regs[9];
    short scanline = 0;
    short scan_cycle = 21;

    int inrange_sprites = 0;
    uint8_t scanline_sprites[8][4];

    uint8_t rgb_palette[64][3];

    //internal registers
    uint16_t v, t;
    uint8_t x;
    bool w;

    short address_latch_state = 0;

    bool nmi_occurred = false;
    bool nmi_output = false;

    uint8_t vram_read_buffer = 0;

    Sprite sprite(uint8_t index);
    uint8_t tile_col_at_pixel(Tile tile, int dx, int dy, bool flip_x, bool flip_y);
};


#endif //PPU_H
