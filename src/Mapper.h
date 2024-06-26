#pragma once

#include "Cartridge.h"
#include "PPU.h"
#include "CPU.h"

enum MIRRORING
{
	Horizontal, Vertical, OneScreen_LB, OneScreen_HB, FourScreen
};

enum class SCType
{
	NONE,
	RICOH_2A03,
	SUNSOFT_5B
};

class SoundChip;

// === MAPPER 0 (NROM) ===

class Mapper
{
public:
	explicit Mapper( Cartridge *cart );

	~Mapper() = default;

	virtual u8 *map_cpu( u16 addr );

	virtual u8 *map_ppu( u16 addr );

	virtual void handle_write( u8 data, u16 addr )
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

	SoundChip *get_sound_chip()
	{
		return sound_chip;
	}

	void set_sound_chip( SoundChip *chip )
	{
		sound_chip = chip;
	}

	virtual const SCType get_sound_chip_type()
	{
		return SCType::NONE;
	}

protected:
	Cartridge *cartridge;
	MIRRORING mirroring = Horizontal;
	u8 bank_prg = 0;
	u8 bank_chr = 0;
	u32 prg_size;
	u32 chr_size;

	u8 *cpu_mem;
	u8 *ppu_mem;
	u8 *prg_rom;
	u8 *chr_rom;
	u8 *prg_ram;
	u8 *chr_ram;
	u8 *nt_ram;

	bool force_mirroring = false;
	bool irq_pending = false;
	bool irq_disable = false;

	SoundChip *sound_chip = nullptr;
};

// === MAPPER 1 (MMC1) ===

class Mapper1 : public Mapper
{
public:
	explicit Mapper1( Cartridge *cart ) : Mapper( cart )
	{
	};

	u8 *map_cpu( u16 address ) override;

	u8 *map_ppu( u16 address ) override;

	void handle_write( u8 data, u16 addr ) override;

private:
	u8 shifter = 0x80;

	u8 bankmode_prg = 3;
	u8 bankmode_chr = 1;

	u8 bank_chr_2 = 0;

	bool bank_prg_256k = 0;

	long last_write = 0;
};

// === MAPPER 2 (UNROM) ===

class Mapper2 : public Mapper
{
public:
	explicit Mapper2( Cartridge *cart ) : Mapper( cart )
	{
	};

	u8 *map_cpu( u16 address ) override
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

	void handle_write( u8 data, u16 addr ) override
	{
		if ( addr < 0x8000 )
		{
			return;
		}
		bank_prg = data & ((prg_size / 0x4000) - 1);
	}
};

// === MAPPER 3 (CNROM) ===

// TODO emulate bus conflicts (e.g. Cybernoid)
class Mapper3 : public Mapper
{
public:
	explicit Mapper3( Cartridge *cart ) : Mapper( cart )
	{
	};

	u8 *map_ppu( u16 address ) override
	{
		if ( address >= 0x2000 )
		{
			return Mapper::map_ppu( address );
		}
		return chr_rom + bank_chr * 0x2000 + address;
	}

	void handle_write( u8 data, u16 addr ) override
	{
		if ( addr < 0x8000 )
		{
			return;
		}
		bank_chr = data & 0x3;//((chr_size / 0x2000) - 1);
	}
};

// === MAPPER 4 (MMC3) ===

class Mapper4 : public Mapper
{
public:
	explicit Mapper4( Cartridge *cart ) : Mapper( cart )
	{};

	u8 *map_cpu( u16 address ) override;

	u8 *map_ppu( u16 address ) override;

	void handle_write( u8 data, u16 addr ) override;

	void handle_ppu_rising_edge() override;

private:
	bool bankmode_prg = 0;
	bool bankmode_chr = 0;

	u8 bank_prg_2 = 0;
	u8 bank_chr_2kb[ 2 ] = { 0 };
	u8 bank_chr_1kb[ 4 ] = { 0 };

	u8 bank_select = 0;

	u8 irq_counter = 0;
	u8 irq_reload_val = 0;
	bool irq_reload = false;
};

// === MAPPER 7 (AxROM) ===

class Mapper7 : public Mapper
{
public:
	explicit Mapper7( Cartridge *cart ) : Mapper( cart )
	{};

	u8 *map_cpu( u16 address ) override;

	void handle_write( u8 data, u16 addr ) override;
};

// === MAPPER 11 (Color Dreams) ===

class Mapper11 : public Mapper
{
public:
	explicit Mapper11( Cartridge *cart ) : Mapper( cart )
	{};

	u8 *map_cpu( u16 address ) override;

	u8 *map_ppu( u16 address ) override;

	void handle_write( u8 data, u16 addr ) override;
};

// === MAPPER 69 (FME-7) ===

class Mapper69 : public Mapper
{
public:
	explicit Mapper69( Cartridge *cart ) : Mapper( cart )
	{};

	u8 *map_cpu( u16 address ) override;

	u8 *map_ppu( u16 address ) override;

	void handle_write( u8 data, u16 addr ) override;

	void handle_cpu_cycle() override;

	const SCType get_sound_chip_type() override
	{
		return SCType::SUNSOFT_5B;
	}

private:
	u16 irq_counter = 0;
	bool irq_counter_enable = false;

	u8 prg_banks[ 4 ] = { 0 };
	u8 chr_banks[ 8 ] = { 0 };
	bool prg_bank0_ram = false;

	u8 command = 0;

	u8 sound_chip_reg = 0x0;
	bool sound_chip_write_enable = true;
};

// === MAPPER 184 (Sunsoft-1) ===

class Mapper184 : public Mapper
{
public:
	explicit Mapper184( Cartridge *cart ) : Mapper( cart )
	{};

	u8 *map_ppu( u16 address ) override;

	void handle_write( u8 data, u16 addr ) override;

private:
	u8 bank_chr_2 = 1;
};

// === MAPPER 228 (Active Enterprises) ===

class Mapper228 : public Mapper
{
public:
	explicit Mapper228( Cartridge *cart ) : Mapper( cart )
	{};

	u8 *map_cpu( u16 address ) override;

	u8 *map_ppu( u16 address ) override;

	void handle_write( u8 data, u16 addr ) override;

private:
	bool prg_bankmode = 0;
	u8 prg_chip = 0;
};