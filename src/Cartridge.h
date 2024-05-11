#pragma once

#include <fstream>
#include <ios>
#include <string>
#include <vector>
#include "Memory.h"
#include "Component.h"

enum class CartError
{
	NONE,
	FILE_OPEN,
	FILE_READ,
	FILE_HEADER,
	MAPPER,
};

class Mapper;

class Cartridge : public Component
{
public:
	Cartridge() : Component()
	{
	};

	~Cartridge();

	bool load();

	bool open_file( const nfdchar_t *filename );

	std::ifstream &get_file()
	{
		return file;
	}

	u32 get_prg_size() const
	{
		return prg_size;
	}

	u32 get_chr_size() const
	{
		return chr_size == 0 ? chr_ram_size : chr_size;
	}

	Memory *get_prg_rom()
	{
		return &prg_rom;
	}

	Memory *get_chr_rom()
	{
		return &chr_rom;
	}

	Memory *get_prg_ram()
	{
		return &prg_ram;
	}

	Memory *get_chr_ram()
	{
		return &chr_ram;
	}

	Memory *get_nt_ram()
	{
		return &nt_ram;
	}

	Mapper *get_mapper()
	{
		return mapper;
	}

	void dump_sram();

	std::string get_error();

private:
	bool read_next( u32 bytes = 1 );

	bool read_next( u8 *into, u32 bytes = 1 );

	bool read_next( Memory &into, u32 start, u32 bytes = 1 );

	bool read_header();

	void print_metadata();

	void load_sram();

private:
	static const int BUFFER_SIZE = 16;
	u8 buffer[BUFFER_SIZE];
	std::ifstream file;
	u32 pos = 0;
	uint64_t prg_size;
	uint64_t chr_size;
	u16 mapper_num;
	bool flags[2][4];

	Memory prg_rom;
	Memory chr_rom;

	Memory prg_ram;
	Memory chr_ram;
	Mapper *mapper = nullptr;

	Memory nt_ram;

	bool battery_ram = false;

	CartError err = CartError::NONE;

	// === NES2.0 ===
	bool nes2 = false;
	u32 prg_ram_size = 0;
	u32 chr_ram_size = 0;
	u8 submapper_num = 0;
};
