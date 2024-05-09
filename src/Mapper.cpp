#include "Mapper.h"
#include "APU/SoundChip.h"

Mapper::Mapper( Cartridge *cart ) : cartridge( cart )
{
	prg_size = cartridge->get_prg_size();
	chr_size = cartridge->get_chr_size();
	cpu_mem = cartridge->get_nes()->get_cpu()->get_mem()->get_mem();
	ppu_mem = cartridge->get_nes()->get_ppu()->get_mem()->get_mem();
	prg_rom = cartridge->get_prg_rom()->get_mem();
	chr_rom = cartridge->get_chr_rom()->get_mem();
	prg_ram = cartridge->get_prg_ram()->get_mem();
	chr_ram = cartridge->get_chr_ram()->get_mem();
	mirroring = Horizontal;
}

// === MAPPER 0 ===

u8 *Mapper::map_cpu( u16 addr )
{
	if ( addr < 0x2000 )
	{
		return cpu_mem + (addr % 0x800);
	}
	else if ( addr >= 0x8000 )
	{
		if ( prg_size != 0x8000 )
		{
			addr &= ~0x4000;
		}
		return prg_rom + (addr - 0x8000);
	}
	else if ( addr >= 0x6000 && prg_ram != nullptr )
	{
		return prg_ram + (addr - 0x6000);
	}
	else
	{
		return nullptr;
	}
}

u8 *Mapper::map_ppu( u16 addr )
{
	if ( addr >= 0x3F00 )
	{
		return nullptr;
	}
	if ( addr < 0x2000 )
	{
		return chr_rom == nullptr ? chr_ram + addr : chr_rom + addr;
	}
	addr &= ~0x1000; // $3000-$3FFF is a mirror of $2000-$2FFF
	addr -= 0x2000;
	u8 *base = ppu_mem;
	switch ( mirroring )
	{
		case Horizontal:
			return base + (addr & ~0x400) % 0x800 + 0x400 * (addr / 0x800);
		case Vertical:
			return base + (addr & ~0x800);
		case OneScreen_LB:
			return base + addr % 0x400;
		case OneScreen_HB:
			return base + addr % 0x400 + 0x400;
		case FourScreen:
			if ( addr < 0x2000 )
			{
				return base + addr;
			}
			else
			{
				return chr_ram + addr;
			}
		default:
			return nullptr;
	}
}

// === MAPPER 1 (MMC1) ===

u8 *Mapper1::map_cpu( u16 address )
{
	if ( address < 0x8000 )
	{
		return Mapper::map_cpu( address );
	}
	switch ( bankmode_prg )
	{
		case 0:
		case 1:
			return prg_rom + (bank_prg & ~0x1) * 0x8000 + (address - 0x8000) + bank_prg_256k * 0x40000;
		case 2:
			if ( address < 0xC000 )
			{
				return prg_rom + (address - 0x8000) + bank_prg_256k * 0x40000;
			}
			else
			{
				return prg_rom + bank_prg * 0x4000 + (address - 0xC000) + bank_prg_256k * 0x40000;
			}
		case 3:
		default:
			if ( address >= 0xC000 )
			{
				return prg_rom + (std::clamp( (int)prg_size, 0, 0x40000 ) - 0x4000) + (address - 0xC000) + bank_prg_256k * 0x40000;
			}
			else
			{
				return prg_rom + bank_prg * 0x4000 + (address - 0x8000) + bank_prg_256k * 0x40000;
			}
	}
}

u8 *Mapper1::map_ppu( u16 address )
{
	if ( address >= 0x2000 )
	{
		return Mapper::map_ppu( address );
	}

	u8 *chr_mem = chr_ram == nullptr ? chr_rom : chr_ram;
	if ( bankmode_chr )
	{
		if ( address < 0x1000 )
		{
			return chr_mem + bank_chr * 0x1000 + address;
		}
		else
		{
			return chr_mem + bank_chr_2 * 0x1000 + (address - 0x1000);
		}
	}
	else
	{
		return chr_mem + (bank_chr & ~0x1) * 0x2000 + address;
	}
}

void Mapper1::handle_write( u8 data, u16 addr )
{
	long cyc = cartridge->get_nes()->get_cpu()->get_cycle();
	if ( addr < 0x8000 )
	{
		return;
	}
	if ( data & 0x80 )
	{
		shifter = 0x80;
		bankmode_prg = 3;
		last_write = cyc;
		return;
	} // clear the shift register if bit 7 is set

	if ( cyc - last_write > 1 )
	{ // ignore consecutive writes
		shifter = (shifter >> 1) | ((data & 0x1) << 7); // shift right with bit 0 of data

		if ( shifter & 0x4 )
		{ // if bit 2 is 1, the shifter is full and we update the relevant bank
			u8 shifter_out = (shifter & 0xF8) >> 3;

			if ( addr >= 0xE000 )
			{ // PRG bank switch
				bank_prg = shifter_out & 0xF;
				if ( !(bankmode_prg & 0x2) )
				{
					bank_prg &= ~0x1;
				} // if 32KB prg banking, ignore low bit

			}
			else if ( addr >= 0xC000 )
			{ // CHR bank-2 switch
				bank_chr_2 = shifter_out % (chr_size / 0x1000);
				if ( prg_size == 0x80000 )
				{
					bank_prg_256k = (shifter_out >> 4) & 0x1;
				}
			}
			else if ( addr >= 0xA000 )
			{ // CHR bank-1 switch
				bank_chr = shifter_out % (chr_size / 0x1000);
				if ( prg_size == 0x80000 )
				{
					bank_prg_256k = (shifter_out >> 4) & 0x1;
				}
			}
			else
			{ // control
				switch ( shifter_out & 0x3 )
				{
					case 0:
						set_mirroring( OneScreen_LB );
						break;
					case 1:
						set_mirroring( OneScreen_HB );
						break;
					case 2:
						set_mirroring( Vertical );
						break;
					case 3:
						set_mirroring( Horizontal );
						break;
				}

				bankmode_prg = (shifter_out >> 2) & 0x3;
				bankmode_chr = (shifter_out >> 4) & 0x1;

			}

			shifter = 0x80;
		}
	}

	last_write = cyc;
}

// === MAPPER 4 (MMC3) ===

u8 *Mapper4::map_cpu( u16 address )
{
	if ( address < 0x8000 )
	{
		return Mapper::map_cpu( address );
	}
	
	if ( address >= 0xE000 )
	{
		return prg_rom + (prg_size - 0x2000) + (address - 0xE000);
	}
	else if ( address >= 0xC000 )
	{
		if ( !bankmode_prg )
		{
			return prg_rom + (prg_size - 0x4000) + (address - 0xC000);
		}
		else
		{
			return prg_rom + (bank_prg * 0x2000) + (address - 0xC000);
		}
	}
	else if ( address >= 0xA000 )
	{
		return prg_rom + (bank_prg_2 * 0x2000) + (address - 0xA000);
	}
	else
	{
		if ( !bankmode_prg )
		{
			return prg_rom + (bank_prg * 0x2000) + (address - 0x8000);
		}
		else
		{
			return prg_rom + (prg_size - 0x4000) + (address - 0x8000);
		}
	}
}

u8 *Mapper4::map_ppu( u16 address )
{
	if ( address >= 0x2000 )
	{
		return Mapper::map_ppu( address );
	}

	u8 *chr_mem = chr_ram == nullptr ? chr_rom : chr_ram;

	if ( bankmode_chr )
	{
		address ^= 0x1000;
	}

	if ( address < 0x1000 )
	{
		return chr_mem + (bank_chr_2kb[ address / 0x800 ] * 0x400) + (address % 0x800);
	}
	else
	{
		return chr_mem + (bank_chr_1kb[ (address - 0x1000) / 0x400 ] * 0x400) + (address % 0x400);
	}
}

void Mapper4::handle_write( u8 data, u16 addr )
{
	if ( addr < 0x8000 )
	{
		return;
	}

	bool even = addr % 2 == 0;
	
	if ( addr < 0xA000 )			// $8000-$9FFF: bank switching
	{
		if ( even )						// even: bank select
		{
			bank_select = data & 0x7;
			bankmode_prg = (data >> 0x6) & 0x1;
			bankmode_chr = (data >> 0x7) & 0x1;
		}
		else							// odd: bank data
		{
			if ( bank_select <= 1 )
			{
				data &= ~0x1;				// ignore lowest bit for 2kb banks (even only)
				bank_chr_2kb[ bank_select ] = data;
			}
			else if ( bank_select <= 5 )
			{
				bank_chr_1kb[ bank_select - 2 ] = data;
			}
			else							
			{
				data &= 0x3F;				// ignore top two bits for prg banks (max 512kb)
				if ( bank_select - 6 == 0 )
				{
					bank_prg = data;
				}
				else
				{
					bank_prg_2 = data;
				}
			}
		}
	}
	else if ( addr < 0xC000 )		// $A000-$BFFF: mirroring/ram protect
	{
		if ( even )						// even: select mirroring
		{
			if ( data & 0x1 )
			{
				set_mirroring( Horizontal );
			}
			else
			{
				set_mirroring( Vertical );
			}
		}
		else							// odd: prg ram protection (ignore for now)
		{
		}
	}
	else if ( addr < 0xE000 )		// $C000-$DFFF: irq reload control
	{
		if ( even )						// even: irq counter reload value
		{
			irq_reload_val = data;
		}
		else							// odd: clear irq counter and set reload flag
		{
			irq_counter = 0;
			irq_reload = true;
		}
	}
	else							// $E000-$FFFF: irq disable/enable
	{
		irq_disable = even;
		if ( irq_disable )
		{
			irq_pending = false;
		}
	}
}

void Mapper4::handle_ppu_rising_edge()
{
	if ( irq_counter == 0 || irq_reload )
	{
		irq_counter = irq_reload_val;
		irq_reload = false;
	}
	else
	{
		--irq_counter;
	}

	if ( irq_counter == 0 && !irq_disable )
	{
		irq_pending = true;
	}
}

// === MAPPER 7 (AxROM) ===

u8 *Mapper7::map_cpu( u16 address )
{
	if ( address < 0x8000 )
	{
		return Mapper::map_cpu( address );
	}

	return prg_rom + (bank_prg * 0x8000) + (address - 0x8000);
}

void Mapper7::handle_write( u8 data, u16 addr )
{
	if ( addr < 0x8000 )
	{
		return;
	}

	bank_prg = data & 0x7;
	set_mirroring( ((data >> 4) & 0x1) ? OneScreen_HB : OneScreen_LB );
}

// === MAPPER 11 (Color Dreams) ===

u8 *Mapper11::map_cpu( u16 address )
{
	if ( address < 0x8000 )
	{
		return Mapper::map_cpu( address );
	}

	return prg_rom + (bank_prg * 0x8000) + (address - 0x8000);
}

u8 *Mapper11::map_ppu( u16 address )
{
	if ( address >= 0x2000 )
	{
		return Mapper::map_ppu( address );
	}

	return chr_rom + (bank_chr * 0x2000) + address;
}

void Mapper11::handle_write( u8 data, u16 addr )
{
	if ( addr < 0x8000 )
	{
		return;
	}

	bank_prg = data & 0x3;
	bank_chr = data >> 4;
}

// === MAPPER 69 (FME-7) ===

u8 *Mapper69::map_cpu( u16 address )
{
	if ( address < 0x6000 )
	{
		return Mapper::map_cpu( address );
	}

	if ( address >= 0xE000 )
	{
		return prg_rom + (prg_size - 0x2000) + (address - 0xE000);
	}
	else if ( address >= 0x8000 )
	{
		return prg_rom + (prg_banks[ 1 + (address - 0x8000) / 0x2000 ] * 0x2000) + (address % 0x2000);
	}
	else
	{
		u16 offset = (prg_banks[ 0 ] * 0x2000) + (address - 0x6000);
		return prg_bank0_ram ? prg_ram + offset : prg_rom + offset;
	}
}

u8 *Mapper69::map_ppu( u16 address )
{
	if ( address >= 0x2000 )
	{
		return Mapper::map_ppu( address );
	}

	u8 *chr_mem = chr_ram == nullptr ? chr_rom : chr_ram;

	return chr_mem + (chr_banks[ address / 0x400 ] * 0x400) + (address % 0x400);
}

void Mapper69::handle_write( u8 data, u16 addr )
{
	if ( addr < 0x8000 )
	{
		return;
	}

	if ( addr < 0xA000 )			// $8000-$9FFF: command register
	{
		command = data & 0xF;
	}
	else if (addr < 0xC000)			// $A000-$BFFF: parameter register
	{
		if ( command <= 0x7 )			// $0-$7: CHR bank select
		{
			chr_banks[ command ] = data % (chr_size / 0x400);
		}
		else if ( command <= 0xB )		// $8-$B: PRG bank select
		{
			prg_banks[ command - 8 ] = (data & 0x1F) % (prg_size / 0x2000);

			if ( command == 8 )
			{
				prg_bank0_ram = (data >> 6) & 0x1;
			}
		}
		else if ( command == 0xC )		// $C: mirroring select
		{
			switch ( data & 0x3 )
			{
				case 0:
					set_mirroring( Vertical );
					break;
				case 1:
					set_mirroring( Horizontal );
					break;
				case 2: 
					set_mirroring( OneScreen_LB );
					break;
				case 3:
					set_mirroring( OneScreen_HB );
					break;
			}
		}
		else if ( command == 0xD )		// $D: IRQ control
		{
			irq_pending = false;
			irq_disable = !(data & 0x1);
			irq_counter_enable = (data >> 7) & 0x1;
		}
		else if ( command == 0xE )		// $E: IRQ counter low byte
		{
			irq_counter = (irq_counter & 0xFF00) | data;
		}
		else							// $F: IRQ counter high byte
		{
			irq_counter = (irq_counter & 0x00FF) | (((u16)data) << 8);
		}
	}
	else if ( addr < 0xE000 )
	{
		sound_chip_reg = GET_BITS( data, 0, 4 );
		sound_chip_write_enable = GET_BITS( data, 4, 4 ) != 0;
	}
	else
	{
		if ( sound_chip == nullptr )
		{
			return;
		}
		sound_chip->write_reg( sound_chip_reg, data );
	}
}

void Mapper69::handle_cpu_cycle()
{
	if ( irq_counter_enable )
	{
		if ( --irq_counter == 0xFFFF && !irq_disable )
		{
			irq_pending = true;
		}
	}
}

// === MAPPER 228 (Active Enterprises) ===

u8 *Mapper228::map_cpu( u16 address )
{
	if ( address < 0x8000 )
	{
		return Mapper::map_cpu( address );
	}

	if ( prg_chip == 2 )
	{
		return prg_rom;
	}

	u32 offset = 0x80000 * prg_chip;
	if ( prg_chip == 3 )
	{
		offset -= 0x80000;
	}

	u8 bank = bank_prg;
	if ( !prg_bankmode )
	{
		bank = address >= 0xC000 ? (bank | 0x1) : (bank & ~0x1);
	}

	return prg_rom + offset + (bank * 0x4000) + (address % 0x4000);
}

u8 *Mapper228::map_ppu( u16 address )
{
	if ( address >= 0x2000 )
	{
		return Mapper::map_ppu( address );
	}

	return chr_rom + (bank_chr * 0x2000) + address;
}

void Mapper228::handle_write( u8 data, u16 addr )
{
	if ( addr < 0x8000 )
	{
		return;
	}

	bank_chr = (data & 0x3) | ((addr & 0xF) << 2);
	bank_prg = (addr >> 6) & 0x1F;
	prg_bankmode = (addr >> 5) & 0x1;
	prg_chip = (addr >> 11) & 0x3;
	set_mirroring( (addr >> 13) & 0x1 ? Horizontal : Vertical );
}