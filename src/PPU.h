#pragma once

#include "Processor.h"

enum PPU_REG
{
	PPUCTRL, PPUMASK, PPUSTATUS, OAMADDR, OAMDATA, PPUSCROLL, PPUADDR, PPUDATA
};

typedef uint8_t *Sprite;
enum SPRITE
{
	Y, TILE, ATTR, X
};

typedef uint8_t Tile[8][2];

class PPU : public Processor
{
public:
	PPU();

	~PPU()
	{
		std::fill( oam, oam + 256, 0 );
		std::fill( palette, palette + 32, 0x1D );
	}

	virtual void reset() override;

	virtual void init() override;

	virtual bool run() override;

	uint8_t read_reg( uint8_t reg_id, int cycle );

	uint8_t read_reg( uint8_t reg_id, int cycle, bool physical_read );

	bool write_reg( uint8_t reg_id, uint8_t value, int cycle, bool physical_write );

	void write_oam( uint8_t byte, uint8_t data );

	void set_palette( std::string palFileName );

	void set_default_palette();

	short get_x() const
	{
		return scan_cycle;
	}

	short get_y() const
	{
		return scanline;
	}

	long get_frame() const
	{
		return frame;
	}

	uint16_t get_v() const
	{
		return v;
	}

	uint16_t get_t() const
	{
		return t;
	}

	uint8_t get_finex() const
	{
		return x;
	}

	uint16_t get_w() const
	{
		return w;
	}

	void output_pt();

	void output_nt();

protected:
	uint8_t read( int addr ) override;

	bool write( uint16_t addr, uint8_t data ) override;

private:
	static uint16_t mirror_palette_addr( uint16_t addr );

	uint8_t oam[256];
	uint8_t oam2[32];
	uint8_t palette[32];
	uint8_t regs[8];

	uint8_t io_bus;

	short scanline = 0;
	short scan_cycle = 3;
	long frame = 1;

	int inrange_sprites = 0;
	uint8_t scanline_sprites[8][4];

	uint8_t rgb_palette[64][3];

	//internal registers
	uint16_t v, t;
	uint8_t x;
	bool w; // address latch

	uint16_t tile_shift_regs[2];
	uint16_t tile_attr_shift_regs[2];
	bool attr_latch[2];

	bool nmi_occurred = false;
	bool nmi_output = false;

	uint8_t vram_read_buffer = 0;

	Sprite sprite( uint8_t index );

	static uint8_t tile_col_at_pixel( Tile tile, int dx, int dy, bool flip_x, bool flip_y );

	uint8_t *bgr_base_rgb();

	uint8_t *col_to_rgb( uint8_t attr, uint8_t col, bool spr );

	bool pt_shown = false;
};
