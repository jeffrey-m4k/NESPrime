#pragma once

#include "Processor.h"

enum PPU_REG
{
	PPUCTRL, PPUMASK, PPUSTATUS, OAMADDR, OAMDATA, PPUSCROLL, PPUADDR, PPUDATA
};

typedef u8 *Sprite;
enum SPRITE
{
	Y, TILE, ATTR, X
};

typedef u8 Tile[8][2];

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

	u8 read_reg( u8 reg_id, int cycle );

	u8 read_reg( u8 reg_id, int cycle, bool physical_read );

	bool write_reg( u8 reg_id, u8 value, int cycle, bool physical_write );

	void write_oam( u8 byte, u8 data );

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

	u16 get_v() const
	{
		return v;
	}

	u16 get_t() const
	{
		return t;
	}

	u8 get_finex() const
	{
		return x;
	}

	u16 get_w() const
	{
		return w;
	}

	void output_pt();

	void output_nt();

protected:
	u8 read( int addr ) override;

	bool write( u16 addr, u8 data ) override;

private:
	static u16 mirror_palette_addr( u16 addr );

	u8 oam[256];
	u8 oam2[32];
	u8 palette[32];
	u8 regs[8];

	u8 io_bus;

	short scanline = 0;
	short scan_cycle = 3;
	long frame = 1;

	int inrange_sprites = 0;
	u8 scanline_sprites[8][4];

	u8 rgb_palette[64][3];

	//internal registers
	u16 v, t;
	u8 x;
	bool w; // address latch

	u16 tile_shift_regs[2];
	u16 tile_attr_shift_regs[2];
	bool attr_latch[2];

	bool nmi_occurred = false;
	bool nmi_output = false;

	u8 vram_read_buffer = 0;

	Sprite sprite( u8 index );

	static u8 tile_col_at_pixel( Tile tile, int dx, int dy, bool flip_x, bool flip_y );

	u8 *bgr_base_rgb();

	u8 *col_to_rgb( u8 attr, u8 col, bool spr );

	void set_a12( u16 addr )
	{
		if ( !a12_set )
		{
			a12 = addr & 0x1000;
			a12_set = true;
		}
	}

	void check_rising_edge();

	bool pt_shown = false;

	bool a12 = 0;
	bool a12_set = false;
	short a12_low_cycles = 0;
	bool a12_rising_filter = false;
	short m2_counter = 0;
};
