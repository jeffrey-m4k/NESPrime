#pragma once

#include <algorithm>
#include <string>
#include <fstream>
#include <nfd.h>

class CPU;

class PPU;

class Cartridge;

class Display;

class IO;

class APU;

class UI;

class NES
{
public:
	NES();

	~NES();

	void run();

	bool run( const nfdchar_t *fn );

	void check_refresh();

	void tick( bool do_cpu, int times );

	void reset();

	void kill()
	{
		quit = true;
	}

	void set_emu_speed(float val) 
	{
		EMU_SPEED = std::clamp( (double)val, 0.0, 3.0 );
	}

	void mod_emu_speed(float mod) 
	{
		EMU_SPEED = std::clamp( (double)EMU_SPEED + mod, 0.0, 3.0 );
	}

	float get_emu_speed()
	{
		return EMU_SPEED;
	}

	CPU *get_cpu()
	{
		return cpu;
	};

	PPU *get_ppu()
	{
		return ppu;
	};

	Cartridge *get_cart()
	{
		return cart;
	}

	Display *get_display()
	{
		return display;
	}

	IO *get_io()
	{
		return io;
	}

	APU *get_apu()
	{
		return apu;
	}

	UI *get_ui()
	{
		return ui;
	}

	void set_cpu( CPU *cpu );

	void set_ppu( PPU *ppu );

	void set_cart( Cartridge *cart );

	void set_display( Display *display );

	void set_io( IO *io );

	void set_apu( APU *apu );

	void set_ui( UI *ui );

	bool DEBUG_PATTERNTABLE = false;
	bool DEBUG_NAMETABLE = false;
	bool DEBUG_APU = false;

	std::ofstream out;
	std::string filename;
private:
	Cartridge *cart;
	CPU *cpu;
	PPU *ppu;
	Display *display;
	IO *io;
	APU *apu;
	UI *ui;

	static constexpr int CPS = 21477272;
	static constexpr int FPS = 60;
	float EMU_SPEED;

	float cycles_delta = 0;

	long clock = 0;
	bool quit = false;

	void dump_ram();
};
