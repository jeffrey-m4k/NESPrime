#pragma once

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

	static const int CPS = 21477272;
	static constexpr float CPF = CPS / 60.0;
	float cycles_delta = 0;

	long clock = 0;
	bool quit = false;
};
