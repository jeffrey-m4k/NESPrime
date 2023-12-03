#include "PPU.h"
#include "CPU.h"
#include "Display.h"
#include "util.h"
#include <cmath>

PPU::PPU() : Processor() {
    this->PPU::reset();
    this->regs[PPUSTATUS] = 0x0;
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
    if (!pt_shown && nes->get_display() != nullptr) output_pt();

    if (scanline == 241 && scan_cycle == 1) {
        // Set the v-blank flag on dot 1 of line 241
        regs[PPUSTATUS] |= 0x80;
        nmi_occurred = true;
        if (nmi_output) {
            nes->get_cpu()->trigger_nmi();
        }
    } else if (scanline == -1 && scan_cycle == 1) {
        // Clear the v-blank flag and sprite overflow flag on dot 1 of pre-render line
        regs[PPUSTATUS] &= ~0xE0;
        nmi_occurred = false;
    }

    bool grayscale = regs[PPUMASK] >> 0;
    bool render_bgr_l = regs[PPUMASK] >> 1;
    bool render_spr_l = regs[PPUMASK] >> 2;
    bool render_bgr = regs[PPUMASK] >> 3;
    bool render_spr = regs[PPUMASK] >> 4;

    bool do_render = render_bgr || render_spr;

    if (do_render) {
        if (scanline <= 240) {
            uint8_t *bgr_rgb = nullptr;
            uint8_t *spr_rgb = nullptr;
            bool spr_priority = false;

            // Get bgr color
            if (scan_cycle >= 1 && scan_cycle <= 256 && scanline >= 0 && scanline <= 239) {
                if (render_bgr && (scan_cycle > 8 || render_bgr_l)) {
                    uint16_t pattern = tile_shift_regs[0];
                    uint8_t attr = tile_attr_shift_regs[0];

                    uint8_t col = (((pattern >> 15) & 0x1) << 1) | ((pattern >> 7) & 0x1);

                    int quadrant_shift;
                    int x_relative = (v & 0x1F) % 2;
                    int y_relative = ((v >> 5) & 0x1F) % 2;
                    if (x_relative == 0) {
                        if (y_relative == 0) quadrant_shift = 0;
                        else quadrant_shift = 4;
                    } else {
                        if (y_relative == 0) quadrant_shift = 2;
                        else quadrant_shift = 6;
                    }
                    bgr_rgb = col == 0 ? rgb_palette[read(0x3F00)] : col_to_rgb(attr >> quadrant_shift, col, false);

                    /*if (scanline == 0) {
                        for (int i = 0; i < 0x20; i++) {
                            print_hex(std::cout, read(0x3F00 + i)); std::cout<<"|"; print_hex(std::cout,palette[i]);
                            std::cout << " ";
                            if (i==0x0f) std::cout<<"\n";
                        }
                        std::cout << "\n\n";
                    }*/
                }
            }

            tile_shift_regs[0] <<= 1;

            // Get spr color
            bool sprite_0 = false;
            if (render_spr && (scan_cycle > 8 || render_spr_l)) {
                Tile tile;
                uint16_t pattern_table = (regs[PPUCTRL] >> 3) & 0x1 ? 0x1000 : 0x0;

                bool tall_sprites = (regs[PPUCTRL] >> 5) & 0x1;
                if (scanline >= 0 && scanline <= 239 && scan_cycle >= 1 && scan_cycle <= 256) {
                    for (int s = 0; scanline != 0 && s < inrange_sprites; s++) {
                        Sprite sprite = scanline_sprites[s];
                        int dx = (scan_cycle - 1) - sprite[SPRITE::X];
                        int dy = scanline - (sprite[SPRITE::Y] + 1);
                        if (dx >= 0 && dx <= 7 && dy >= 0 && dy <= 7) {
                            uint8_t tile_num = sprite[SPRITE::TILE];
                            // TODO support 8x16 sprites
                            for (int b = 0; b < 16; b++) {
                                tile[b % 8][b / 8] = read(pattern_table + tile_num * 16 + b);
                            }
                            uint8_t col_at_pos = tile_col_at_pixel(tile, dx, dy, (sprite[SPRITE::ATTR] >> 6) & 0x1,
                                                                   (sprite[SPRITE::ATTR] >> 7) & 0x1);

                            if (col_at_pos != 0) {
                                spr_rgb = col_to_rgb(sprite[SPRITE::ATTR], col_at_pos, true);
                                spr_priority = ((sprite[SPRITE::ATTR] >> 5) & 0x1) == 0;

                                bool is_sprite0 = true;
                                if ((regs[PPUSTATUS] & 0x40) != 0x40) {
                                    for (int i=0; i<4; i++) {
                                        if (sprite[i] != this->sprite(0)[i]) is_sprite0 = false;
                                        break;
                                    }
                                }
                                sprite_0 = is_sprite0;
                                break;
                            }
                        }
                    }

                }
            }

            if (bgr_rgb != nullptr || spr_rgb != nullptr) {
                uint8_t *final_rgb;

                if (bgr_rgb == nullptr) final_rgb = spr_rgb;
                else if (spr_rgb == nullptr) final_rgb = bgr_rgb;
                else {
                    final_rgb = spr_priority ? spr_rgb : bgr_rgb;
                    if (sprite_0) {
                        regs[PPUSTATUS] |= 0x40;
                    }
                }

                nes->get_display()->set_pixel_buffer(scan_cycle - 1, scanline, final_rgb);
            }
        }

        // Sprite evaluation
        if (scanline != -1) {
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

        // Move vertical bits from temp VRAM address at end of vblank
        if (scanline == -1 && scan_cycle >= 280 && scan_cycle <= 304) {
            v = (v & ~0x7BE0) | (t & 0x7BE0);
        }

        // Every 8 dots, increment coarse X and update shift registers
        if ((scan_cycle-1) % 8 == 7 && (scan_cycle >= 328 || scan_cycle <= 256)) {
            tile_shift_regs[0] = tile_shift_regs[1];
            tile_attr_shift_regs[0] = tile_attr_shift_regs[1];

            uint16_t next_tile_addr = 0x2000 | (v & 0x0FFF);
            uint16_t next_attr_addr = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07);

            uint8_t next_tile = read(next_tile_addr);
            uint8_t next_attr = read(next_attr_addr);

            uint16_t pattern_addr = 0x1000 * ((regs[PPUCTRL] >> 4) & 0x1) + ((uint16_t)next_tile << 4) + ((v & 0x7000) >> 12);
            tile_shift_regs[1] = read(pattern_addr);
            tile_attr_shift_regs[1] = read(next_attr_addr);

            if ((v & 0x001F) == 31) {
                v &= ~0x001F;
                v ^= 0x0400;
            } else v += 1;
        }

        // Increment fine Y at end of line and wrap around
        if (scan_cycle == 256) {
            if ((v & 0x7000) != 0x7000)
                v += 0x1000;
            else {
                v &= ~0x7000;
                int y = (v>>5) & 0x1F;
                if (y == 29) {
                    y = 0;
                    v ^= 0x800;
                }
                else if (y == 31) y = 0;
                else y += 1;

                v = (v & ~0x03E0) | (y << 5);
            }
        } else if (scan_cycle == 257) { // Move horizontal bits from temp VRAM address
            v = (v & ~0x041F) | (t & 0x041F);
        }
    } else if ((v & 0x3F00) == 0x3F00 && scan_cycle >= 1 && scan_cycle <= 256 && scanline >= 0 && scanline <= 239) {
        // Background palette hack
        nes->get_display()->set_pixel_buffer(scan_cycle - 1, scanline, rgb_palette[read(mirror_palette_addr(v))]);
    }

    // Display test
//    uint8_t test[3] = {static_cast<uint8_t>((128 + sin(cycle)*63)*0.25), 0, static_cast<uint8_t>((128+cos(nes->get_clock()/1000000.0)*63)*0.25)};
//    if (scan_cycle <= 255 && scanline <= 239) nes->get_display()->set_pixel_buffer(scan_cycle - 1, scanline, test);


    //std::cout << (cycle / 60) << " ";



    scan_cycle++;
    if (scan_cycle > 340 || (do_render && scan_cycle == 340 && scanline == -1 && frame % 2 != 0)) { scan_cycle = 0; scanline++; }
    if (scanline > 260) { scanline = -1; nes->get_display()->push_buffer(); }
    if (scanline == 240 && scan_cycle == 0) frame++;

    return true;
}

uint16_t PPU::mirror_palette_addr(uint16_t addr) {
    if ((addr & 0x3F00) == 0x3F00 && (((addr >> 4) & 0xF)) % 2 != 0 && (addr & 0xF) % 4 == 0)
        addr -= 0x10;
    return addr;
}

uint8_t PPU::read_reg(uint8_t reg_id, int cycle) {
    return read_reg(reg_id, cycle, true);
}

uint8_t PPU::read_reg(uint8_t reg_id, int cycle, bool physical_read) {
    if (physical_read) {
        if (reg_id == PPUDATA) {
            if (physical_read) {
                io_bus = vram_read_buffer;
                vram_read_buffer = read(mirror_palette_addr(v & 0x3FFF));
                v += ((regs[PPUCTRL] >> 2) & 0x1) ? 32 : 1;
            } else return read(mirror_palette_addr(v));
        } else if (reg_id == OAMDATA) { // TODO
        } else if (reg_id == PPUSTATUS) {
            io_bus = (io_bus & ~0xE0) | (regs[PPUSTATUS] & 0xE0);
            regs[PPUSTATUS] &= ~0x80;
            nmi_occurred = false;
            w = false;
        }
        return io_bus;
    } else return regs[reg_id];
}

bool PPU::write_reg(uint8_t reg_id, uint8_t value, int cycle, bool physical_write) {
    /*if (cycle < 29658 && (reg_id == PPUCTRL || reg_id == PPUMASK || reg_id == PPUSCROLL || reg_id == PPUADDR))
        return false;
    else*/
    switch (reg_id) {
        case PPUCTRL:
            t = (t & ~0xC00) | ((value & 0x3) << 10);
            break;
        case PPUADDR:
            if (!w) {
                t = (t & 0x00FF) | ((value & 0x3F) << 8) & ~0x4000;
            } else {
                t = (t & 0xFF00) | value;
                v = t;
            }
            w = !w;
            break;
        case PPUSCROLL:
            if (!w) {
                t = (t & 0xFFE0) | ((value >> 3) & 0x1F);
                x = value & 0x7;
            } else {
                t = (t & 0x1F) | (((value >> 3) & 0x1F) << 5) | ((value & 0x7) << 12);
            }
            w = !w;
            break;
        case PPUDATA:
            write(mirror_palette_addr(v & 0x3FFF), value);
            v += ((regs[PPUCTRL] >> 2) & 0x1) ? 32 : 1;
            break;
        default:
            break;
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

uint8_t* PPU::col_to_rgb(uint8_t attr, uint8_t col, bool spr) {
    uint8_t* rgb = rgb_palette[read(mirror_palette_addr(0x3F01 + (attr & 0x3) * 4 + (col-1) + spr*0x10))];
    return rgb;
}

void PPU::output_pt() {
    for (int i=0; i<2; i++) {
        for (int n=0; n<256; n++) {
            uint16_t pattern_addr = (i << 12) + (n << 4);
            for(int y=0; y<8; y++) {
                uint8_t p1 = read(pattern_addr+y);
                uint8_t p2 = read(pattern_addr+y+8);

                for (int cx=0; cx<8; cx++) {
                    uint8_t col = ((p1>>(7-cx) & 0x1)) | ((p2>>(7-cx) & 0x1) << 1);
                    //uint8_t rgb[3] = {static_cast<uint8_t>(col*85), static_cast<uint8_t>(col*85), static_cast<uint8_t>(col*85)};
                    if (col != 0) {
                        uint8_t rgb[3] = {0, 0, 0};
                        rgb[col-1] = 255;
                        nes->get_display()->write_pt_pixel(n, cx, y, i, rgb);
                    }
                }
            }
        }
    }
    nes->get_display()->update_pt();
    pt_shown = true;
}