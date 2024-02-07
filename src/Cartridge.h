#pragma once

#include <fstream>
#include <ios>
#include <string>
#include <vector>
#include <cstdint>
#include "Memory.h"
#include "Component.h"

class Mapper;

class Cartridge : public Component
{
public:
	Cartridge() : Component()
	{
	};

	~Cartridge();

	void load();

	bool open_file( const nfdchar_t *filename );

	std::ifstream &get_file()
	{
		return file;
	}

	uint32_t get_prg_size() const
	{
		return prg_size;
	}

	uint32_t get_chr_size() const
	{
		return chr_size;
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

	Mapper *get_mapper()
	{
		return mapper;
	}

	void dump_sram();

private:
	bool read_next( uint32_t bytes = 1 );

	bool read_next( uint8_t *into, uint32_t bytes = 1 );

	bool read_next( Memory &into, uint32_t start, uint32_t bytes = 1 );

	bool read_header();

	void print_metadata();

	void load_sram();

private:
	static const int BUFFER_SIZE = 16;
	uint8_t buffer[BUFFER_SIZE];
	std::ifstream file;
	uint32_t pos = 0;
	uint32_t prg_size;
	uint32_t chr_size;
	uint8_t mapper_num;
	bool flags[2][4];

	Memory prg_rom;
	Memory chr_rom;

	Memory prg_ram;
	Memory chr_ram;
	Mapper *mapper = nullptr;

	bool battery_ram = false;
};
