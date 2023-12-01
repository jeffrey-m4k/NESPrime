#include "PPU.h"
#include "CPU.h"
#include "Display.h"
#include <cmath>

PPU::PPU() : Processor() {
    this->PPU::reset();
    this->regs[PPUSTATUS] = 0xA0;
    this->set_palette("classic.pal");
}

void PPU::reset() {
    this->aspace.clear();
    uint8_t* memory = this->mem.get_mem();
    switch (nt_mirror) {
        //TODO limit mirroring to 3EFF
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
    //write_reg(PPUMASK, 0x1E, 100000); //force enable all rendering
}

void PPU::init() {}
bool PPU::run() {
    if (scanline == 241 && scan_cycle == 1) {
        // Set the v-blank flag on dot 1 of line 241
        regs[PPUSTATUS] |= 0x80;
        nmi_occurred = true;
    } else if (scanline == 261 && scan_cycle == 1) {
        // Clear the v-blank flag and sprite overflow flag on dot 1 of pre-render line
        regs[PPUSTATUS] &= ~0xE0;
        nmi_occurred = false;
    }
    if (nmi_occurred && nmi_output) {
        nes->get_cpu()->trigger_nmi();
    }

    bool grayscale = regs[PPUMASK] >> 0;
    bool render_bgr_l = regs[PPUMASK] >> 1;
    bool render_spr_l = regs[PPUMASK] >> 2;
    bool render_bgr = regs[PPUMASK] >> 3;
    bool render_spr = regs[PPUMASK] >> 4;

    bool do_render = ((regs[PPUMASK] >> 1) & 0xF) != 0;

    if (do_render) {
        // Sprite evaluation
        if (scanline != 261) {
            // Clear oam2 from cycles 1-64
            if (scan_cycle >= 1 && scan_cycle <= 64 && scan_cycle % 2 == 0) {
                oam2[scan_cycle / 2 - 1] = 0xFF;
            } else if (scan_cycle == 65) {
                inrange_sprites = 0;
                //TODO maybe add optional support for bypassing 8-sprite limit
                int n = 0;
                for (; n < 64; n++) {
                    Sprite spr = sprite(n);
                    uint8_t y = spr[SPRITE::Y];
                    if (y <= scanline && (scanline - y) < 8) {
                        for (int i = 0; i < 4; i++) {
                            oam2[inrange_sprites * 4 + i] = spr[i];
                        }
                        inrange_sprites++;
                    }
                    if (inrange_sprites == 8) break;
                }
                // We emulate the hardware bug and treat each byte of the remaining sprites as a y-coordinate
                if (n < 64) {
                    for (; n < 64; n++) {
                        for (int m = 0; m < 4; m++) {
                            uint8_t faux_y = oam[n * 4 + m];
                            if (faux_y <= scanline && (scanline - faux_y) < 8) {
                                regs[PPUSTATUS] |= 0x20;
                            } else {
                                n++;
                                m++;
                            }
                        }
                    }
                }
            } else if (scan_cycle == 257) {
                for (int s = 0; s < inrange_sprites; s++) {
                    for (int i = 0; i < 4; i++) {
                        scanline_sprites[s][i] = oam2[s*4+i];
                    }
                }
            }

        }

        // Rendering
        Tile tile;
        uint16_t pattern_table = (regs[PPUCTRL] >> 3) & 0x1 ? 0x1000 : 0x0;

        bool tall_sprites = (regs[PPUCTRL] >> 5) & 0x1;
        if (scanline >= 0 && scanline <= 239 && scan_cycle >= 1 && scan_cycle <= 256) {
            for (int s = 0; scanline != 0 && s < inrange_sprites; s++) {
                Sprite sprite = scanline_sprites[s];
                int dx = (scan_cycle-1) - sprite[SPRITE::X];
                int dy = scanline - (sprite[SPRITE::Y]+1);
                if (dx >= 0 && dx <= 7 && dy >= 0 && dy <= 7) {
                    uint8_t tile_num = sprite[SPRITE::TILE];
                    // TODO support 8x16 sprites
                    for (int b = 0; b < 16; b++) {
                        tile[b % 8][b / 8] = read(pattern_table + tile_num * 16 + b);
                    }
                    uint8_t col_at_pos = tile_col_at_pixel(tile, dx, dy, (sprite[SPRITE::ATTR] >> 6) & 0x1, (sprite[SPRITE::ATTR] >> 7) & 0x1);
                    /*for (int ay=0; ay<8; ay++) {
                        for (int ax=0; ax<8; ax++) {
                            uint8_t col = tile_col_at_pixel(tile,ax,ay,0,0);
                            uint8_t* rgb = rgb_palette[read(0x3F11 + (sprite[SPRITE::ATTR] & 0x3) * 4 + col)];
                            std::cout<<"("<<+rgb[0]<<","<<+rgb[1]<<","<<+rgb[2]<<") ";
                        }
                        std::cout<<"\n";
                    }
                    std::cout<<"\n\n\n";*/
                    if (col_at_pos != 0) {
                        uint8_t* rgb = rgb_palette[read(0x3F11 + (sprite[SPRITE::ATTR] & 0x3) * 4 + col_at_pos)];
                        nes->get_display()->set_pixel_buffer(scan_cycle - 1, scanline, rgb);
                        break;
                    }
                }
            }

        }

    }

    // Display test
//    uint8_t test[3] = {static_cast<uint8_t>((128 + sin(cycle)*63)*0.25), 0, static_cast<uint8_t>((128+cos(nes->get_clock()/1000000.0)*63)*0.25)};
//    if (scan_cycle <= 255 && scanline <= 239) nes->get_display()->set_pixel_buffer(scan_cycle - 1, scanline, test);


    //std::cout << (cycle / 60) << " ";
    scan_cycle++;
    if (scan_cycle > 340) { scan_cycle = 0; scanline++; }
    if (scanline > 261) { scanline = 0; nes->get_display()->push_buffer(); }

    return true;
}

uint16_t PPU::mirror_palette_addr(uint16_t addr) {
    if ((addr & 0x3F00) == 0x3F00 && (((addr >> 4) & 0xF)) % 2 != 0)
        addr -= 0x10;
    return addr;
}

uint8_t PPU::read_reg(uint8_t reg_id, int cycle) {
    return read_reg(reg_id, cycle, true);
}

uint8_t PPU::read_reg(uint8_t reg_id, int cycle, bool physical_read) {
    if (reg_id == PPUDATA) {
        if (physical_read) {
            uint8_t temp = vram_read_buffer;
            vram_read_buffer = read(v);
            v += ((regs[PPUCTRL] >> 2) & 0x1) ? 32 : 1;
            return temp;
        }
        else return read(mirror_palette_addr(v));
    }
    return regs[reg_id];
}

bool PPU::write_reg(uint8_t reg_id, uint8_t value, int cycle) {
    /*if (cycle < 29658 && (reg_id == PPUCTRL || reg_id == PPUMASK || reg_id == PPUSCROLL || reg_id == PPUADDR))
        return false;
    else*/
    if (reg_id == PPUADDR) {
        if (address_latch_state == 0) {
            v = value << 8;
            address_latch_state = 1;
        } else if (address_latch_state == 1) {
            v = v | value;
            address_latch_state = 2;
        }
    } else if (reg_id == PPUDATA) {
        write(mirror_palette_addr(v), value);
        v += ((regs[PPUCTRL] >> 2) & 0x1) ? 32 : 1;
    }
        regs[reg_id] = value;
        nmi_occurred = (regs[PPUSTATUS] >> 7) & 0x1;
        nmi_output = (regs[PPUCTRL] >> 7) & 0x1;
    return true;
}

void PPU::write_oam(uint8_t byte, uint8_t data) {
    oam[byte] = data;
}

Sprite PPU::sprite(uint8_t index) {
    if (index < 0 || index > 63) return nullptr;
    return &oam[index*4];
}

uint8_t PPU::tile_col_at_pixel(Tile tile, int dx, int dy, bool flip_x, bool flip_y) {
    uint8_t x = flip_x ? (7-dx) : dx;
    uint8_t y = flip_y ? (7-dy) : dy;

    bool val1 = tile[y][0] >> (7-x) & 0x1;
    bool val2 = tile[y][1] >> (7-x) & 0x1;
    uint8_t color = (val2 << 1) | val1;

    return color;
}

void PPU::set_palette(std::string palFileName) {
    std::ifstream palFile(palFileName);
    for (int col = 0; !palFile.eof() && col < 64; col++) {
        palFile.read((char*)rgb_palette[col], 3);
    }
}