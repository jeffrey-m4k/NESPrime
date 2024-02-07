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
	{}

	virtual void handle_ppu_rising_edge()
	{}

	virtual void handle_cpu_cycle()
	{}

	void set_mirroring( MIRRORING mirr )
	{
		if ( !force_mirroring )
		{
			mirroring = mirr;
		}
	}
	
	void set_force_mirroring( bool force )
	{
		force_mirroring = force;
	}

	MIRRORING get_mirroring()
	{
		return mirroring;
	}

	bool has_prg_ram()
	{
		return prg_ram != nullptr;
	}

	bool check_irq()
	{
		return irq_pending && !irq_disable;
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

	bool force_mirroring = false;
	bool irq_pending = false;
	bool irq_disable = false;
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


class Mapper4 : public Mapper
{
public:
	explicit Mapper4( Cartridge *cart ) : Mapper( cart )
	{};

	uint8_t *map_cpu( uint16_t address ) override;

	uint8_t *map_ppu( uint16_t address ) override;

	void handle_write( uint8_t data, uint16_t addr ) override;

	void handle_ppu_rising_edge() override;

private:
	bool bankmode_prg = 0;
	bool bankmode_chr = 0;

	uint8_t bank_prg_2 = 0;
	uint8_t bank_chr_2kb[ 2 ] = { 0 };
	uint8_t bank_chr_1kb[ 4 ] = { 0 };

	uint8_t bank_select = 0;

	uint8_t irq_counter = 0;
	uint8_t irq_reload_val = 0;
	bool irq_reload = false;
};


class Mapper7 : public Mapper
{
public:
	explicit Mapper7( Cartridge *cart ) : Mapper( cart )
	{};

	uint8_t *map_cpu( uint16_t address ) override;

	void handle_write( uint8_t data, uint16_t addr ) override;
};


class Mapper11 : public Mapper
{
public:
	explicit Mapper11( Cartridge *cart ) : Mapper( cart )
	{};

	uint8_t *map_cpu( uint16_t address ) override;

	uint8_t *map_ppu( uint16_t address ) override;

	void handle_write( uint8_t data, uint16_t addr ) override;
};


class Mapper69 : public Mapper
{
public:
	explicit Mapper69( Cartridge *cart ) : Mapper( cart ) 
	{};

	uint8_t *map_cpu( uint16_t address ) override;

	uint8_t *map_ppu( uint16_t address ) override;

	void handle_write( uint8_t data, uint16_t addr ) override;

	void handle_cpu_cycle() override;

private:
	uint16_t irq_counter = 0;
	bool irq_counter_enable = false;

	uint8_t prg_banks[ 4 ];
	uint8_t chr_banks[ 8 ];
	bool prg_bank0_ram = false;

	uint8_t command = 0;
};


class Mapper228 : public Mapper
{
public:
	explicit Mapper228( Cartridge *cart ) : Mapper( cart )
	{};

	uint8_t *map_cpu( uint16_t address ) override;

	uint8_t *map_ppu( uint16_t address ) override;

	void handle_write( uint8_t data, uint16_t addr ) override;

private:
	bool prg_bankmode = 0;
	uint8_t prg_chip = 0;
};