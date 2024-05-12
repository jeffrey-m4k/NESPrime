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
	reg.p = 0x24; // Initialize STATUS register to 00100100
}

bool CPU::run()
{
	if ( oam_cycles > 0 )
	{
		skip_cycles( 1, READ );
		int i = 256 - oam_cycles--;
		nes->get_ppu()->write_oam( i, read( create_address( i, memory_regs[0x14]), false));
		skip_cycles( 1, WRITE );
		return true;
	}

	oper_set = false;
	polled_interrupt = false;
	pending_interrupt = -1;
	suppress_skip_cycles = false;

	if ( mapper->check_irq() )
	{
		trigger_irq();
	}

	exec( read( reg.pc ) );

	poll_interrupt();
	PIN_NMI = false;
	PIN_IRQ = false;

	if ( pending_interrupt == INTERRUPT_TYPE::NMI || pending_interrupt == INTERRUPT_TYPE::IRQ )
	{
		interrupt( (INTERRUPT_TYPE)pending_interrupt );
	}

	return true;
}

bool CPU::poll_interrupt()
{
	if ( !polled_interrupt )
	{
		polled_interrupt = true;

		if ( PIN_NMI )
		{
			pending_interrupt = INTERRUPT_TYPE::NMI;
			return true;
		}
		else if ( PIN_IRQ && !(reg.p & STATUS::i) )
		{
			pending_interrupt = INTERRUPT_TYPE::IRQ;
			return true;
		}
	}

	return false;
}

void CPU::interrupt( INTERRUPT_TYPE type )
{
	if ( type != BREAK )
	{
		skip_cycles( 2, READ );
	}
	u8 push_p = type == BREAK ? reg.p | STATUS::b : reg.p & ~STATUS::b;
	u16 vector = type == NMI ? 0xFFFA : 0xFFFE;
	u16 push_addr = type == BREAK ? ++reg.pc + 1 : reg.pc;
	if ( type == IRQ )
	{
		reg.p |= STATUS::i;
	}
	push_address( push_addr );
	push_stack( push_p );
	reg.pc = read_address( vector );
}

void CPU::skip_cycles( int num, CYCLE type )
{
	for ( int i = 0; i < num; i++ )
	{
		state = type;
		nes->tick( false, 12 - 1 * (i == num - 1) );
		mapper->handle_cpu_cycle();
		cycle++;
	}
}

u8 CPU::oper()
{
	if ( !oper_set )
	{
		ops[0] = read( addrs[0] );
		oper_set = true;
	}
	return ops[0];
}

void CPU::exec( const u8 opcode )
{
	inc_pc = true;

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
	bool page_sensitive = ((curr_op.cycles & PAGE_SENSITIVE) >> 4) & 1;

	bool phys = true;
	if ( curr_op.id == Op::STA || curr_op.id == Op::STX || curr_op.id == Op::STY || curr_op.id == Op::INC )
	{
		phys = false;
	}
	switch ( curr_op.mode )
	{
		case Accumulator:
		case Implicit:
			break;
		case Immediate:
			addrs[0] = ++reg.pc;
			break;
		case Absolute:
		{
			u8 lo = read( ++reg.pc );
			u8 hi = read( ++reg.pc );
			addrs[0] = create_address( lo, hi );
			break;
		}
		case ZeroPage:
		{
			u8 lo = read( ++reg.pc );
			addrs[0] = lo;
			break;
		}
		case IndexedAbsoluteX:
		case IndexedAbsoluteY:
		{
			u8 lo = read( ++reg.pc );
			u8 hi = read( ++reg.pc );
			u16 addr = create_address( lo, hi );
			addrs[0] = addr + (curr_op.mode == IndexedAbsoluteX ? reg.x : reg.y);
			if ( !same_page( addr, addrs[0] ) && page_sensitive || !page_sensitive )
			{
				skip_cycles( 1, READ );
			}
			break;
		}
		case IndexedZeroX:
		case IndexedZeroY:
		{
			u8 lo = read( ++reg.pc );
			addrs[0] = (u8) (lo + (curr_op.mode == IndexedZeroX ? reg.x : reg.y));
			skip_cycles( 1, READ );
			break;
		}
		case Indirect:
		{
			u8 lo = read( ++reg.pc );
			u8 hi = read( ++reg.pc );
			u16 at = create_address( lo, hi );
			addrs[0] = create_address( read( at ), read( (u8) (at + 1) | (at & 0xFF00) ) );
			// Jump address wraps around in indirect mode due to 6502 bug
			break;
		}
		case IndexedIndirectX:
		{
			u8 base = read( ++reg.pc );
			auto addr = (u8) (base + reg.x);
			addrs[0] = create_address( read( addr ), read( (u8) (addr + 1) ) );
			skip_cycles( 1, READ );
			break;
		}
		case IndexedIndirectY:
		{
			u8 base = read( ++reg.pc );
			u16 addr = create_address( read( base ), read( (u8) (base + 1) ) );
			addrs[0] = addr + reg.y;
			if ( !same_page( addr, addrs[0] ) && page_sensitive || !page_sensitive )
			{
				skip_cycles( 1, READ );
			}
			break;
		}
		case Relative:
			offset = (i8) read( ++reg.pc );
			addrs[0] = reg.pc + 1 + offset;
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

void CPU::set_value_status( const u8 val )
{
	set_status( STATUS::z, val == 0 );
	set_status( STATUS::n, (bool) (val >> 7) );
}

void CPU::push_stack( const u8 byte )
{
	write( create_address( reg.s--, 0x01 ), byte );
}

u8 CPU::pop_stack()
{
	u8 data = read( create_address( ++reg.s, 0x01 ) );
	return data;
}

u8 CPU::peek_stack( const u8 bytes )
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

void CPU::compare( const u8 a, const u8 b )
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
		u8 diff = a - b;
		set_status( STATUS::n, (bool) (diff >> 7) );
		set_status( STATUS::c, a > b );
	}
}

void CPU::push_address( const u16 addr )
{
	push_stack( (u8) ((addr & 0xFF00) >> 8) );
	push_stack( (u8) (addr & 0x00FF) );
}

u16 CPU::pop_address()
{
	u8 a = pop_stack();
	u8 b = pop_stack();
	return create_address( a, b );
}

u16 CPU::read_address( const u16 addr )
{
	return create_address( read( addr ), read( addr + 1 ) );
}

u16 CPU::create_address( const u8 lo, const u8 hi )
{
	return ((u16) hi) << 8 | lo;
}

bool CPU::same_page( const u16 addr1, const u16 addr2 )
{
	return (addr1 >> 8 == addr2 >> 8);
}

u8 CPU::shift_right( u8 byte )
{
	set_status( STATUS::c, (bool) (byte & 0x1) );
	byte = byte >> 1;
	return byte;
}

u8 CPU::shift_left( u8 byte )
{
	set_status( STATUS::c, (bool) ((byte >> 7) & 0x1) );
	byte = byte << 1;
	return byte;
}

u8 CPU::rot_right( u8 byte )
{
	u8 c = get_status( STATUS::c );
	byte = shift_right( byte ) | (c << 7);
	return byte;
}

u8 CPU::rot_left( u8 byte )
{
	u8 c = get_status( STATUS::c );
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
	u16 sum = reg.acc + ops[0] + carry;
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
	u8 test = oper();
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
	poll_interrupt();
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
	u8 status = pop_stack();
	bool b = get_status( STATUS::b );
	bool bit_5 = get_status( STATUS::bit_5 );

	poll_interrupt();
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
	u8 status = pop_stack();
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
	poll_interrupt();
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

u8 CPU::read( int addr )
{
	return read( addr, true );
}

u8 CPU::read( int addr, bool physical_read )
{
	if ( physical_read )
	{
		skip_cycles( 1, READ );
	}
	if ( addr >= 0x2000 && addr <= 0x3FFF )
	{
		u8 ppureg = nes->get_ppu()->read_reg( addr % 8, cycle, physical_read );
		return ppureg;
	}
	else if ( addr >= 0x4000 && addr < 0x4018 )
	{
		if ( addr == 0x4015 )
		{
			return nes->get_apu()->read_status();
		}
		else if ( addr == 0x4016 )
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

bool CPU::write( const u16 addr, const u8 data )
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
		skip_cycles( 1 + (cycle % 2 != 0), HALT );
		oam_cycles = 256;
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
		if ( addr < 0x8000 )
		{
			u8 *write_addr = mapper->map_cpu( addr );
			*write_addr = data;

			if ( addr >= 0x6000 )
			{
				u8 *ram_addr = nes->get_cart()->get_prg_ram()->get_mem();
				mapper->ram_highest = std::max( mapper->ram_highest, std::distance( ram_addr, write_addr ) );
			}
		}
	}
	mapper->handle_write( data, addr );
	return true;
}

void CPU::dummy_write( const u16 addr, const u8 data )
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