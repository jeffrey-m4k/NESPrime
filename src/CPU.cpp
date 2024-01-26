#include <algorithm>
#include <iostream>
#include <iomanip>
#include "util.h"
#include "CPU.h"
#include "NES.h"
#include "PPU.h"
#include "IO.h"
#include "APU/APU.h"

CPU::CPU() : Processor()
{
	this->CPU::reset();
}

void CPU::reset()
{
}

void CPU::init()
{
	cycle = 0;
	nes->out.fill( '0' );

	skip_cycles( 3, READ );
	reg.s = 0x00; // Initialize stack head pointer
	for ( int i = 0; i < 3; i++ )
	{
		skip_cycles( 1, READ );
		reg.s--;
	}

	reg.pc = create_address( read( 0xFFFC ), read( 0xFFFD ) );
	//reg.pc = 0xC000;// uncomment for nestest
	reg.p = 0x24; // Initialize STATUS register to 00100100

	nes->out << "\n\n===== CPU INITIALIZED =====";
}

bool CPU::run()
{
	oper_set = false;
	polled_interrupt = false;
	suppress_skip_cycles = false;

	if constexpr( logging )
	{
		log_state();
	}

	uint8_t opcode = read( reg.pc );

	if constexpr( logging )
	{
		print_hex( nes->out, reg.pc, "  " );
	}

	exec( opcode );

	if constexpr( logging )
	{
		nes->out << setw( 35 ) << std::left << regs_log.str();
	}

	if ( PIN_NMI )
	{
		interrupt( NMI );
		if constexpr( logging )
		{
			nes->out << "\n!!! NMI TRIGGERED !!!";
		}
	}

	return true;
}

void CPU::interrupt( INTERRUPT_TYPE type )
{
	if ( type != BREAK )
	{
		skip_cycles( 2, READ );
	}
	uint8_t push_p = type == BREAK ? reg.p | STATUS::b : reg.p & ~STATUS::b;
	uint16_t vector = type == NMI ? 0xFFFA : 0xFFFE;
	uint16_t push_addr = type == BREAK ? ++reg.pc + 1 : reg.pc;
	reg.p |= STATUS::i;
	push_address( push_addr );
	push_stack( push_p );
	reg.pc = read_address( vector );
	PIN_NMI = false;
}

void CPU::skip_cycles( int num, CYCLE type )
{
	for ( int i = 0; i < num; i++ )
	{
		state = type;
		nes->tick( false, 12 - 1 * (i == num - 1) );
		cycle++;
	}
}

uint8_t CPU::oper()
{
	if ( !oper_set )
	{
		ops[0] = read( addrs[0] );
		oper_set = true;
	}
	return ops[0];
}

void CPU::exec( const uint8_t opcode )
{
	if constexpr( logging )
	{
		oss.str( "" );
	}

	auto it = OPCODES.find( opcode );
	if ( it == OPCODES.end() )
	{
		if constexpr( logging )
		{
			nes->out << "\n\nError: unknown opcode! ";
			print_hex( nes->out, opcode );
		}
		exit;
	}

	curr_op = it->second;
	inc_pc = true;
	//bool crossed_page = false;
	bool page_sensitive = ((curr_op.cycles & PAGE_SENSITIVE) >> 4) & 1;

	//idle_cycles += (curr_op.cycles & ~PAGE_SENSITIVE);
	if constexpr( logging )
	{
		print_hex( oss, opcode, " " );
	}

	bool phys = true;
	if ( curr_op.id == Op::STA || curr_op.id == Op::STX || curr_op.id == Op::STY || curr_op.id == Op::INC )
	{
		phys = false;
	}
	switch ( curr_op.mode )
	{
		case Accumulator:
			if constexpr( logging )
			{
				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id] << " A";
			}
			break;
		case Implicit:
			if constexpr( logging )
			{
				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id];
			}
			break;
		case Immediate:
			addrs[0] = ++reg.pc;

			if constexpr( logging )
			{
				print_hex( oss, read( addrs[0], false ), " " );

				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id] << " #$";
				print_hex( oss, read( addrs[0], false ) );
			}

			break;
		case Absolute:
		{
			uint8_t lo = read( ++reg.pc );
			uint8_t hi = read( ++reg.pc );
			addrs[0] = create_address( lo, hi );

			if constexpr( logging )
			{
				print_hex( oss, lo, " " );
				print_hex( oss, hi, " " );
				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id] << " $";
				print_hex( oss, addrs[0] );
				oss << " = ";
				print_hex( oss, read( addrs[0], false ) );
			}

			break;
		}
		case ZeroPage:
		{
			uint8_t lo = read( ++reg.pc );
			addrs[0] = lo;

			if constexpr( logging )
			{
				print_hex( oss, lo, " " );
				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id] << " $";
				print_hex( oss, addrs[0] );
				oss << " = ";
				print_hex( oss, read( addrs[0], false ) );
			}

			break;
		}
		case IndexedAbsoluteX:
		case IndexedAbsoluteY:
		{
			uint8_t lo = read( ++reg.pc );
			uint8_t hi = read( ++reg.pc );
			uint16_t addr = create_address( lo, hi );
			addrs[0] = addr + (curr_op.mode == IndexedAbsoluteX ? reg.x : reg.y);
			if ( !same_page( addr, addrs[0] ) && page_sensitive || !page_sensitive )
			{
				skip_cycles( 1, READ );
			}

			if constexpr( logging )
			{
				print_hex( oss, lo, " " );
				print_hex( oss, hi, " " );
				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id] << " $";
				print_hex( oss, addr );
				oss << (curr_op.mode == IndexedAbsoluteX ? ",X" : ",Y") << " @ ";
				print_hex( oss, addrs[0] );
				oss << " = ";
				print_hex( oss, read( addrs[0], false ) );
			}

			break;
		}
		case IndexedZeroX:
		case IndexedZeroY:
		{
			uint8_t lo = read( ++reg.pc );
			addrs[0] = (uint8_t) (lo + (curr_op.mode == IndexedZeroX ? reg.x : reg.y));
			skip_cycles( 1, READ );

			if constexpr( logging )
			{
				print_hex( oss, lo, " " );
				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id] << " $";
				print_hex( oss, lo );
				oss << (curr_op.mode == IndexedZeroX ? ",X" : ",Y") << " @ ";
				print_hex( oss, addrs[0] );
				oss << " = ";
				print_hex( oss, read( addrs[0], false ) );
			}

			break;
		}
		case Indirect:
		{
			uint8_t lo = read( ++reg.pc );
			uint8_t hi = read( ++reg.pc );
			uint16_t at = create_address( lo, hi );
			addrs[0] = create_address( read( at ), read( (uint8_t) (at + 1) | (at & 0xFF00) ) );
			// Jump address wraps around in indirect mode due to 6502 bug

			if constexpr( logging )
			{
				print_hex( oss, lo, " " );
				print_hex( oss, hi, " " );
				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id] << " ($";
				print_hex( oss, at );
				oss << ") = ";
				print_hex( oss, addrs[0] );
			}
			break;
		}
		case IndexedIndirectX:
		{
			uint8_t base = read( ++reg.pc );
			auto addr = (uint8_t) (base + reg.x);
			addrs[0] = create_address( read( addr ), read( (uint8_t) (addr + 1) ) );
			skip_cycles( 1, READ );

			if constexpr( logging )
			{
				print_hex( oss, base, " " );
				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id] << " ($";
				print_hex( oss, base );
				oss << ",X) @ ";
				print_hex( oss, addr );
				oss << " = ";
				print_hex( oss, addrs[0] );
				oss << " = ";
				print_hex( oss, read( addrs[0], false ) );
			}

			break;
		}
		case IndexedIndirectY:
		{
			uint8_t base = read( ++reg.pc );
			uint16_t addr = create_address( read( base ), read( (uint8_t) (base + 1) ) );
			addrs[0] = addr + reg.y;
			if ( !same_page( addr, addrs[0] ) && page_sensitive || !page_sensitive )
			{
				skip_cycles( 1, READ );
			}

			if constexpr( logging )
			{
				print_hex( oss, base, " " );
				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id] << " ($";
				print_hex( oss, base );
				oss << "),Y = ";
				print_hex( oss, addrs[0] );
				oss << " @ ";
				print_hex( oss, addrs[0] );
				oss << " = ";
				print_hex( oss, read( addrs[0], false ) );
			}

			break;
		}
		case Relative:
			offset = (int8_t) read( ++reg.pc );
			addrs[0] = reg.pc + 1 + offset;

			if constexpr( logging )
			{
				print_hex( oss, offset, " " );
				nes->out << setw( 10 ) << std::left << oss.str();
				oss.str( "" );
				oss << OP_NAMES[curr_op.id] << " $";
				print_hex( oss, addrs[0] );
			}

			break;
		default:
			break;
	}

	(*this.*curr_op.op_func)();

	if constexpr( logging )
	{
		nes->out << setw( 30 ) << std::left << oss.str();
	}
	if ( inc_pc )
	{
		reg.pc++;
	}
}

void CPU::set_status( const STATUS status, bool value )
{
	reg.p = value ? reg.p | status : reg.p & ~status;
}

bool CPU::get_status( const STATUS status ) const
{
	return (reg.p & status) == status;
}

void CPU::set_value_status( const uint8_t val )
{
	set_status( STATUS::z, val == 0 );
	set_status( STATUS::n, (bool) (val >> 7) );
}

void CPU::push_stack( const uint8_t byte )
{
	write( create_address( reg.s--, 0x01 ), byte );
}

uint8_t CPU::pop_stack()
{
	uint8_t data = read( create_address( ++reg.s, 0x01 ) );
	return data;
}

uint8_t CPU::peek_stack( const uint8_t bytes )
{
	return read( create_address( reg.s + bytes, 0x01 ) );
}

void CPU::branch( const STATUS status, const bool check_against )
{
	if ( get_status( status ) == check_against )
	{
		if ( !same_page( reg.pc + 1, addrs[0] ) )
		{
			skip_cycles( 2, READ );
		}
		else
		{
			skip_cycles( 1, READ );
		}
		reg.pc = addrs[0];
		inc_pc = false;
	}
}

void CPU::compare( const uint8_t a, const uint8_t b )
{
	if ( a == b )
	{
		set_status( STATUS::n, false );
		set_status( STATUS::z, true );
		set_status( STATUS::c, true );
	}
	else
	{
		set_status( STATUS::z, false );
		uint8_t diff = a - b;
		set_status( STATUS::n, (bool) (diff >> 7) );
		set_status( STATUS::c, a > b );
	}
}

void CPU::push_address( const uint16_t addr )
{
	push_stack( (uint8_t) ((addr & 0xFF00) >> 8) );
	push_stack( (uint8_t) (addr & 0x00FF) );
}

uint16_t CPU::pop_address()
{
	uint8_t a = pop_stack();
	uint8_t b = pop_stack();
	return create_address( a, b );
}

uint16_t CPU::read_address( const uint16_t addr )
{
	return create_address( read( addr ), read( addr + 1 ) );
}

uint16_t CPU::create_address( const uint8_t lo, const uint8_t hi )
{
	return ((uint16_t) hi) << 8 | lo;
}

bool CPU::same_page( const uint16_t addr1, const uint16_t addr2 )
{
	return (addr1 >> 8 == addr2 >> 8);
}

uint8_t CPU::shift_right( uint8_t byte )
{
	set_status( STATUS::c, (bool) (byte & 0x1) );
	byte = byte >> 1;
	return byte;
}

uint8_t CPU::shift_left( uint8_t byte )
{
	set_status( STATUS::c, (bool) ((byte >> 7) & 0x1) );
	byte = byte << 1;
	return byte;
}

uint8_t CPU::rot_right( uint8_t byte )
{
	uint8_t c = get_status( STATUS::c );
	byte = shift_right( byte ) | (c << 7);
	return byte;
}

uint8_t CPU::rot_left( uint8_t byte )
{
	uint8_t c = get_status( STATUS::c );
	byte = shift_left( byte ) | c;
	return byte;
}

void CPU::add_with_carry( bool sub )
{
	oper();
	if ( sub )
	{
		ops[0] = ~ops[0];
	}
	bool carry = get_status( STATUS::c );
	uint16_t sum = reg.acc + ops[0] + carry;
	set_status( STATUS::c, sum > 0xFF );
	set_status( STATUS::v, ~(reg.acc ^ ops[0]) & (reg.acc ^ sum) & 0x80 );
	reg.acc = sum;
	set_value_status( sum );
}

// ==== OPCODES ====
void CPU::ADC()
{
	add_with_carry();
}

void CPU::AND()
{
	reg.acc &= oper();
	set_value_status( reg.acc );
}

void CPU::ASL()
{
	if ( curr_op.mode == Accumulator )
	{
		reg.acc = shift_left( reg.acc );
		skip_cycles( 1, WRITE );
		set_value_status( reg.acc );
	}
	else
	{
		ops[0] = oper();
		ops[1] = shift_left( ops[0] );
		dummy_write( addrs[0], ops[0] );
		write( addrs[0], ops[1] );
		set_value_status( ops[1] );
	}
}

void CPU::BCC()
{
	branch( STATUS::c, false );
}

void CPU::BCS()
{
	branch( STATUS::c, true );
}

void CPU::BEQ()
{
	branch( STATUS::z, true );
}

void CPU::BIT()
{
	uint8_t test = oper();
	set_status( STATUS::n, (bool) (test >> 7 & 1) );
	set_status( STATUS::v, (bool) (test >> 6 & 1) );
	set_status( STATUS::z, (bool) ((test & reg.acc) == 0) );
}

void CPU::BMI()
{
	branch( STATUS::n, true );
}

void CPU::BNE()
{
	branch( STATUS::z, false );
}

void CPU::BPL()
{
	branch( STATUS::n, false );
}

void CPU::BRK()
{
	skip_cycles( 1, READ );
	interrupt( BREAK );
}

void CPU::BVC()
{
	branch( STATUS::v, false );
}

void CPU::BVS()
{
	branch( STATUS::v, true );
}

void CPU::CLC()
{
	set_status( STATUS::c, false );
	skip_cycles( 1, READ );
}

void CPU::CLD()
{
	set_status( STATUS::d, false );
	skip_cycles( 1, READ );
}

void CPU::CLI()
{
	set_status( STATUS::i, false );
	skip_cycles( 1, READ );
}

void CPU::CLV()
{
	set_status( STATUS::v, false );
	skip_cycles( 1, READ );
}

void CPU::CMP()
{
	compare( reg.acc, oper() );
}

void CPU::CPX()
{
	compare( reg.x, oper() );
}

void CPU::CPY()
{
	compare( reg.y, oper() );
}

void CPU::DCP()
{
	DEC();
	CMP();
}

void CPU::DEC()
{
	ops[0] = oper();
	dummy_write( addrs[0], ops[0] );
	write( addrs[0], --ops[0] );
	set_value_status( ops[0] );
}

void CPU::DEX()
{
	set_value_status( --reg.x );
	skip_cycles( 1, READ );
}

void CPU::DEY()
{
	set_value_status( --reg.y );
	skip_cycles( 1, READ );
}

void CPU::EOR()
{
	reg.acc ^= oper();
	set_value_status( reg.acc );
}

void CPU::INC()
{
	ops[0] = oper();
	dummy_write( addrs[0], ops[0] );
	write( addrs[0], ++ops[0] );
	set_value_status( ops[0] );
}

void CPU::INX()
{
	set_value_status( ++reg.x );
	skip_cycles( 1, READ );
}

void CPU::INY()
{
	set_value_status( ++reg.y );
	skip_cycles( 1, READ );
}

void CPU::ISB()
{
	INC();
	SBC();
}

void CPU::JMP()
{
	reg.pc = addrs[0];
	inc_pc = false;
}

void CPU::JSR()
{
	skip_cycles( 1, READ );
	push_address( reg.pc );
	JMP();
}

void CPU::LAX()
{
	oper();
	set_value_status( ops[0] );
	reg.acc = ops[0];
	reg.x = ops[0];
}

void CPU::LDA()
{
	oper();
	set_value_status( ops[0] );
	reg.acc = ops[0];
}

void CPU::LDX()
{
	oper();
	set_value_status( ops[0] );
	reg.x = ops[0];
}

void CPU::LDY()
{
	oper();
	set_value_status( ops[0] );
	reg.y = ops[0];
}

void CPU::LSR()
{
	if ( curr_op.mode == Accumulator )
	{
		reg.acc = shift_right( reg.acc );
		skip_cycles( 1, WRITE );
		set_value_status( reg.acc );
	}
	else
	{
		ops[0] = oper();
		ops[1] = shift_right( ops[0] );
		dummy_write( addrs[0], ops[0] );
		write( addrs[0], ops[1] );
		set_value_status( ops[1] );
	}
}

void CPU::NOP()
{
	skip_cycles( 1, READ );
}

void CPU::ORA()
{
	reg.acc |= oper();
	set_value_status( reg.acc );
}

void CPU::PHA()
{
	skip_cycles( 1, READ );
	push_stack( reg.acc );
}

void CPU::PHP()
{
	skip_cycles( 1, READ );
	push_stack( reg.p | STATUS::b );
}

void CPU::PLA()
{
	skip_cycles( 2, READ );
	reg.acc = pop_stack();
	set_value_status( reg.acc );
}

void CPU::PLP()
{
	skip_cycles( 2, READ );
	uint8_t status = pop_stack();
	bool b = get_status( STATUS::b );
	bool bit_5 = get_status( STATUS::bit_5 );
	reg.p = status;
	set_status( STATUS::b, b );
	set_status( STATUS::bit_5, bit_5 );
}

void CPU::RLA()
{
	ROL();
	AND();
}

void CPU::ROL()
{
	if ( curr_op.mode == Accumulator )
	{
		reg.acc = rot_left( reg.acc );
		skip_cycles( 1, WRITE );
		set_value_status( reg.acc );
	}
	else
	{
		ops[0] = oper();
		ops[1] = rot_left( ops[0] );
		dummy_write( addrs[0], ops[0] );
		write( addrs[0], ops[1] );
		set_value_status( ops[1] );
	}
}

void CPU::ROR()
{
	if ( curr_op.mode == Accumulator )
	{
		reg.acc = rot_right( reg.acc );
		skip_cycles( 1, WRITE );
		set_value_status( reg.acc );
	}
	else
	{
		ops[0] = oper();
		ops[1] = rot_right( ops[0] );
		dummy_write( addrs[0], ops[0] );
		write( addrs[0], ops[1] );
		set_value_status( ops[1] );
	}
}

void CPU::RRA()
{
	ROR();
	ADC();
}

void CPU::RTI()
{
	skip_cycles( 2, READ );
	uint8_t status = pop_stack();
	bool b = get_status( STATUS::b );
	bool bit_5 = get_status( STATUS::bit_5 );
	reg.p = status;
	set_status( STATUS::b, b );
	set_status( STATUS::bit_5, bit_5 );
	reg.pc = pop_address();
	inc_pc = false;
}

void CPU::RTS()
{
	skip_cycles( 2, READ );
	reg.pc = pop_address() + 1;
	skip_cycles( 1, READ );
	inc_pc = false;
}

void CPU::SAX()
{
	write( addrs[0], reg.acc & reg.x );
}

void CPU::SBC()
{
	add_with_carry( true );
}

void CPU::SEC()
{
	set_status( STATUS::c, true );
	skip_cycles( 1, READ );
}

void CPU::SED()
{
	set_status( STATUS::d, true );
	skip_cycles( 1, READ );
}

void CPU::SEI()
{
	set_status( STATUS::i, true );
	skip_cycles( 1, READ );
}

void CPU::SLO()
{
	ASL();
	ORA();
}

void CPU::SRE()
{
	LSR();
	EOR();
}

void CPU::STA()
{
	write( addrs[0], reg.acc );
}

void CPU::STX()
{
	write( addrs[0], reg.x );
}

void CPU::STY()
{
	write( addrs[0], reg.y );
}

void CPU::TAX()
{
	reg.x = reg.acc;
	set_value_status( reg.x );
	skip_cycles( 1, READ );
}

void CPU::TAY()
{
	skip_cycles( 1, READ );
	reg.y = reg.acc;
	set_value_status( reg.y );
}

void CPU::TSX()
{
	skip_cycles( 1, READ );
	reg.x = reg.s;
	set_value_status( reg.x );
}

void CPU::TXA()
{
	skip_cycles( 1, READ );
	reg.acc = reg.x;
	set_value_status( reg.acc );
}

void CPU::TXS()
{
	skip_cycles( 1, READ );
	reg.s = reg.x;
}

void CPU::TYA()
{
	skip_cycles( 1, READ );
	reg.acc = reg.y;
	set_value_status( reg.acc );
}

void CPU::USBC()
{
	SBC();
}

uint8_t CPU::read( int addr )
{
	return read( addr, true );
}

uint8_t CPU::read( int addr, bool physical_read )
{
	if ( physical_read )
	{
		skip_cycles( 1, READ );
	}
	if ( addr >= 0x2000 && addr <= 0x3FFF )
	{
		uint8_t ppureg = nes->get_ppu()->read_reg( addr % 8, cycle, physical_read );
		return ppureg;
	}
	else if ( addr >= 0x4000 && addr < 0x4018 )
	{
		if ( addr == 0x4016 )
		{
			return nes->get_io()->read_joy();
		}
		return memory_regs[addr - 0x4000];
	}
	else if ( addr >= 0x4018 && (addr < 0x6000 || (addr < 0x8000 && !mapper->has_prg_ram())) )
	{
		return 0;
	}
	else
	{
		return *mapper->map_cpu( addr );
	}
}

bool CPU::write( const uint16_t addr, const uint8_t data )
{
	skip_cycles( 1, WRITE );
	if ( addr >= 0x2000 && addr <= 0x3FFF )
	{
		return nes->get_ppu()->write_reg( addr % 8, data, cycle, true );
	}
	else if ( addr == 0x4014 )
	{
		// OAM DMA

		memory_regs[0x14] = data;
		// TODO properly check get/put cycles using APU clock sync (using even-get odd-put for now)
		skip_cycles( 1 + (cycle % 2 != 0), READ );

		for ( int i = 0; i <= 0xFF; i++ )
		{
			nes->get_ppu()->write_oam( i, read( create_address( i, data ), false ) );
		}
		skip_cycles( 512, WRITE );
	}
	else if ( addr >= 0x4000 && addr < 0x4018 )
	{
		nes->get_apu()->write_apu_reg( addr - 0x4000, data );
		memory_regs[addr - 0x4000] = data;
	}
	else if ( addr >= 0x4018 && addr < 0x6000 )
	{ // temp ignore unmapped space
	}
	else
	{
		if ( addr < 0x8000 && nes->get_cart()->get_prg_ram() != nullptr )
		{
			*mapper->map_cpu( addr ) = data;
		}
	}
	mapper->handle_write( data, addr );
	return true;
}

void CPU::dummy_write( const uint16_t addr, const uint8_t data )
{
	skip_cycles( 1, WRITE );
	if ( addr >= 0x8000 )
	{
		mapper->handle_write( data, addr );
	}
}

void CPU::log_state()
{
	regs_log.str("");
	regs_log.clear();
	regs_log << "A:";
	print_hex(regs_log, reg.acc);
	regs_log << " X:";
	print_hex(regs_log, reg.x);
	regs_log << " Y:";
	print_hex(regs_log, reg.y);
	regs_log << " P:";
	print_hex(regs_log, reg.p);
	regs_log << " SP:";
	print_hex(regs_log, reg.s);
	regs_log << " CYC:" << cycle;
	regs_log << " | PPU:";
	for (int i = 0; i < 9; i++)
	{
		print_hex(regs_log, nes->get_ppu()->read_reg(i, cycle, false));
		regs_log << " ";
	}
	regs_log << "CYC:" << nes->get_ppu()->get_y() << "," << nes->get_ppu()->get_x() << ":"
		<< nes->get_ppu()->get_frame();
	regs_log << " | v:";
	print_hex(regs_log, nes->get_ppu()->get_v());
	regs_log << " | t:";
	print_hex(regs_log, nes->get_ppu()->get_t());
	regs_log << " | w:";
	print_hex(regs_log, nes->get_ppu()->get_w());
	regs_log << " | x:";
	print_hex(regs_log, nes->get_ppu()->get_finex());
	nes->out << "\n";
}