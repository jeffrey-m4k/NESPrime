#pragma once

#include <cstdint>
#include "Cartridge.h"
#include "PPU.h"
#include "CPU.h"

enum MIRRORING
{
	Horizontal, Vertical, OneScreen_LB, OneScreen_HB, FourScreen
};

class Mapper
{
public:
	explicit Mapper( Cartridge *cart );

	~Mapper() = default;

	virtual uint8_t *map_cpu( uint16_t addr );

	virtual uint8_t *map_ppu( uint16_t addr );

	virtual void handle_write( uint8_t data, uint16_t addr )
	{
	};

	void set_mirroring( MIRRORING mirr )
	{
		mirroring = mirr;
	}

	MIRRORING get_mirroring()
	{
		return mirroring;
	}

	bool has_prg_ram()
	{
		return prg_ram != nullptr;
	}

protected:
	Cartridge *cartridge;
	MIRRORING mirroring;
	uint8_t bank_prg = 0;
	uint8_t bank_chr = 0;
	uint32_t prg_size;
	uint32_t chr_size;

	uint8_t *cpu_mem;
	uint8_t *ppu_mem;
	uint8_t *prg_rom;
	uint8_t *chr_rom;
	uint8_t *prg_ram;
	uint8_t *chr_ram;
};

class Mapper1 : public Mapper
{
public:
	explicit Mapper1( Cartridge *cart ) : Mapper( cart )
	{
	};

	uint8_t *map_cpu( uint16_t address ) override;

	uint8_t *map_ppu( uint16_t address ) override;

	void handle_write( uint8_t data, uint16_t addr ) override;

private:
	uint8_t shifter = 0x80;

	uint8_t bankmode_prg = 3;
	uint8_t bankmode_chr = 1;

	uint8_t bank_chr_2 = 0;

	long last_write = 0;
};

class Mapper2 : public Mapper
{
public:
	explicit Mapper2( Cartridge *cart ) : Mapper( cart )
	{
	};

	uint8_t *map_cpu( uint16_t address ) override
	{
		if ( address < 0x8000 )
		{
			return Mapper::map_cpu( address );
		}
		if ( address >= 0xC000 )
		{
			return prg_rom + (prg_size - 0x4000) + (address - 0xC000);
		}
		else
		{
			return prg_rom + (bank_prg * 0x4000) + (address - 0x8000);
		}
	}

	void handle_write( uint8_t data, uint16_t addr ) override
	{
		if ( addr < 0x8000 )
		{
			return;
		}
		bank_prg = data & ((prg_size / 0x4000) - 1);
	}
};

// TODO emulate bus conflicts (e.g. Cybernoid)
class Mapper3 : public Mapper
{
public:
	explicit Mapper3( Cartridge *cart ) : Mapper( cart )
	{
	};

	uint8_t *map_ppu( uint16_t address ) override
	{
		if ( address >= 0x2000 )
		{
			return Mapper::map_ppu( address );
		}
		return chr_rom + bank_chr * 0x2000 + address;
	}

	void handle_write( uint8_t data, uint16_t addr ) override
	{
		if ( addr < 0x8000 )
		{
			return;
		}
		bank_chr = data & 0x3;//((chr_size / 0x2000) - 1);
	}
};
