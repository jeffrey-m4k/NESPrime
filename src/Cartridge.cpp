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

bool Cartridge::read_next( const u32 bytes )
{
	return read_next( buffer, bytes );
}

bool Cartridge::read_next( u8 *into, const u32 bytes )
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

bool Cartridge::read_next( Memory &into, const u32 start, const u32 bytes )
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
	if ( file.is_open() )
	{
		return true;
	}
	else
	{
		err = CartError::FILE_OPEN;
		return false;
	}
}

bool Cartridge::read_header()
{
	if ( read_next( 16 ) )
	{
		// Check for valid NES header
		if ( buffer[ 0 ] != 0x4E || buffer[ 1 ] != 0x45 || buffer[ 2 ] != 0x53 || buffer[ 3 ] != 0x1A )
		{
			err = CartError::FILE_HEADER;
			return false;
		}

		// Read PRG and CHR ROM sizes in bytes
		prg_size = buffer[4] * 0x4000;
		prg_ram_size = buffer[8] == 0 ? 0x2000 : buffer[8] * 0x2000;
		chr_size = buffer[5] * 0x2000;
		chr_ram_size = chr_size ? 0 : 0x2000;

		mapper_num = GET_BITS( buffer[ 6 ], 4, 4 ) | (GET_BITS( buffer[ 7 ], 4, 4 ) << 4);

		for ( int i = 0; i < 4; i++ )
		{
			flags[ 0 ][ i ] = GET_BIT( buffer[ 6 ], i );
			flags[ 1 ][ i ] = GET_BIT( buffer[ 7 ], i );
		}

		// NES2.0 format
		if ( GET_BITS( buffer[ 7 ], 2, 2 ) != 0x0 )
		{
			nes2 = true;
			mapper_num |= (GET_BITS( buffer[ 8 ], 0, 4 ) << 8);
			submapper_num = GET_BITS( buffer[ 8 ], 4, 4 );

			// PRG ROM MSB
			if ( GET_BITS( buffer[ 9 ], 0, 4 ) != 0xF )
			{
				prg_size = (buffer[ 4 ] | (GET_BITS( buffer[ 9 ], 0, 4 ) << 8)) * 0x4000;
			}
			else
			{
				prg_size = std::pow( 2, GET_BITS( buffer[ 4 ], 2, 6 ) ) * (GET_BITS( buffer[ 4 ], 0, 2 ) * 2 + 1);
			}

			// CHR ROM MSB
			if ( GET_BITS( buffer[ 9 ], 4, 4 ) != 0xF )
			{
				chr_size = (buffer[ 5 ] | (GET_BITS( buffer[ 9 ], 4, 4 ) << 8)) * 0x2000;
			}
			else
			{
				chr_size = std::pow( 2, GET_BITS( buffer[ 5 ], 2, 6 ) ) * (GET_BITS( buffer[ 5 ], 0, 2 ) * 2 + 1);
			}

			if ( GET_BITS( buffer[ 10 ], 0, 4 ) != 0x0 )
			{
				prg_ram_size = 64 << GET_BITS( buffer[ 10 ], 0, 4 );
			}
			else if( GET_BITS( buffer[ 10 ], 4, 4 ) != 0x0 )
			{
				// TODO make this less hacky - separate volatile from non-volatile RAM if needed
				prg_ram_size = 64 << GET_BITS( buffer[ 10 ], 4, 4 );
			}
			else
			{
				prg_ram_size = 0;
			}

			if ( GET_BITS( buffer[ 11 ], 0, 4 ) != 0x0 )
			{
				chr_ram_size = 64 << GET_BITS( buffer[ 11 ], 0, 4 );
			}
			else if ( GET_BITS( buffer[ 11 ], 4, 4 ) != 0x0 )
			{
				chr_ram_size = 64 << GET_BITS( buffer[ 11 ], 4, 4 );
			}
			else
			{
				chr_ram_size = 0;
			}
		}

		battery_ram = flags[ 0 ][ 1 ];

		flush_hex( nes->out, buffer, 16 );
		return true;
	}
	err = CartError::FILE_READ;
	return false;
}

bool Cartridge::load()
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

		nes->out << endl;

		prg_rom.init( prg_size );
		read_next( prg_rom, 0, prg_size );

		prg_ram.init( prg_ram_size );
		load_sram();

		if ( chr_size )
		{
			chr_rom.init( chr_size );
			read_next( chr_rom, 0, chr_size );
		}
		else
		{
			chr_ram.init( chr_ram_size );
		}

		nt_ram.init( 0x800 );

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
			case 184:
				mapper = new Mapper184( this );
				break;
			case 228:
				mapper = new Mapper228( this );
				break;
			default:
				err = CartError::MAPPER;
				return false;
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

		return true;
	}
	return false;
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
		u8 *ram = prg_ram.get_mem();

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

std::string Cartridge::get_error()
{
	switch ( err )
	{
		case CartError::FILE_OPEN:
			return "Failed to open file";
		case CartError::FILE_READ:
			return "Failed to read file";
		case CartError::FILE_HEADER:
			return "Invalid file header";
		case CartError::MAPPER:
			return "Unsupported mapper: " + std::to_string( mapper_num );
		case CartError::NONE:
		default:
			return "";
	}
}