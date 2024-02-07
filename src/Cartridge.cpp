#include "Cartridge.h"
#include "util.h"
#include "CPU.h"
#include "PPU.h"

#include <iostream>

using std::ios;

Cartridge::~Cartridge()
{
	delete mapper;
}

bool Cartridge::read_next( const uint32_t bytes )
{
	return read_next( buffer, bytes );
}

bool Cartridge::read_next( uint8_t *into, const uint32_t bytes )
{
	// TODO better EOF checking, for now just assuming ROM is formatted correctly
	if ( !file.eof() )
	{
		file.seekg( pos );
		file.read( (char *) into, bytes );
		pos += bytes;
		return true;
	}
	return false;
}

bool Cartridge::read_next( Memory &into, const uint32_t start, const uint32_t bytes )
{
	if ( !file.eof() )
	{
		file.seekg( pos );
		if ( start + bytes > into.get_size() )
		{
			return false;
		}
		file.read( (char *) into.get_mem(), bytes );
		pos += bytes;
		return true;
	}
	return false;
}

bool Cartridge::open_file( const nfdchar_t *filename )
{
	pos = 0;
	nes->out << "===== " << filename << " =====\n\n";
	file.open( filename, ios::in | ios::binary );
	return file.is_open();
}

bool Cartridge::read_header()
{
	if ( read_next( 16 ) )
	{
		// Check for valid NES header
		if ( buffer[0] != 0x4E || buffer[1] != 0x45 || buffer[2] != 0x53 || buffer[3] != 0x1A )
		{
			return false;
		}

		// Check if file is iNES, return false if not
		// TODO add NES2.0 support (low priority)
		if ( (buffer[7] & 0xC) != 0x0 )
		{
			return false;
		}

		// Read PRG and CHR ROM sizes in bytes
		prg_size = buffer[4] * 0x4000;
		chr_size = buffer[5] * 0x2000;

		mapper_num = (buffer[6] >> 4) | (buffer[7] & 0xF0);
		// TODO support other mappers
		for ( int i = 0; i < 4; i++ )
		{
			flags[0][i] = (buffer[6] >> i) & 0x1;
			flags[1][i] = (buffer[7] >> i) & 0x1;
		}

		battery_ram = flags[0][1];

		flush_hex( nes->out, buffer, 16 );
		return true;
	}
	return false;
}

void Cartridge::load()
{
	pos = 0;
	if ( read_header() )
	{
		print_metadata();

		// If trainer is present, skip past it
		if ( flags[0][2] )
		{
			pos += 512;
		}

		prg_rom.init( prg_size );
		read_next( prg_rom, 0, prg_size );

		prg_ram.init( 0x8000 );
		load_sram();

		nes->out << endl;

		if ( chr_size )
		{
			chr_rom.init( chr_size );
			read_next( chr_rom, 0, chr_size );
		}
		else
		{
			chr_size = 0x2000;
			chr_ram.init( chr_size );
		}

		CPU *cpu = nes->get_cpu();
		PPU *ppu = nes->get_ppu();

		switch ( mapper_num )
		{
			case 0:
				mapper = new Mapper( this );
				break;
			case 1:
				mapper = new Mapper1( this );
				break;
			case 2:
				mapper = new Mapper2( this );
				break;
			case 3:
				mapper = new Mapper3( this );
				break;
			case 4:
				mapper = new Mapper4( this );
				break;
			case 7:
				mapper = new Mapper7( this );
				break;
			case 11:
				mapper = new Mapper11( this );
				break;
			case 69:
				mapper = new Mapper69( this );
				break;
			case 228:
				mapper = new Mapper228( this );
				break;
			default:
				exit( EXIT_FAILURE );
		}

		if ( flags[0][3] )
		{
			mapper->set_mirroring( FourScreen );
			mapper->set_force_mirroring( true );
		}
		else
		{
			mapper->set_mirroring( static_cast<MIRRORING>(flags[0][0]) );
		}

		cpu->set_mapper( mapper );
		ppu->set_mapper( mapper );


		// TODO add PlayChoice-10 support (low priority)
	}
	else
	{
		nes->out << "Invalid NES file\n";
	}
}

void Cartridge::print_metadata()
{
	nes->out << "PRG ROM size: " << prg_size << " bytes\n";
	nes->out << "CHR ROM size: " << chr_size << " bytes\n";
	nes->out << "Mapper: " << (int) mapper_num << "\n";
	nes->out << "Flags:\n";
	nes->out << "0: ";
	for ( int i = 0; i < 4; i++ )
	{
		nes->out << flags[0][i] << " ";
	}
	nes->out << "\n1: ";
	for ( int i = 0; i < 4; i++ )
	{
		nes->out << flags[1][i] << " ";
	}
	nes->out << "\n";
}


void Cartridge::dump_sram()
{
	if ( battery_ram )
	{
		std::ofstream save_file( "NESP_Saves/" + nes->filename + ".sav", std::ios::binary);
		uint8_t *ram = prg_ram.get_mem();

		for ( int i = 0; i < 0x2000; ++i )
		{
			save_file << ram[ i ];
		}

		save_file.close();
	}
}

void Cartridge::load_sram()
{
	if ( battery_ram )
	{
		std::ifstream save_file( "NESP_Saves/" + nes->filename + ".sav", ios::binary );
		if ( save_file.good() )
		{
			save_file.seekg( 0, std::ios::end );
			size_t length = save_file.tellg();
			save_file.seekg( 0, std::ios::beg );

			if ( length > 0x8000 )
			{
				length = 0x8000;
			}

			save_file.read( (char *)prg_ram.get_mem(), length );
		}
	}
}