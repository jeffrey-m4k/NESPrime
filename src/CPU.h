#pragma once

#include <sstream>
#include <functional>
#include "Processor.h"
#include "Mapper.h"

enum ADDRESSING_MODE
{
	IndexedZeroX, IndexedZeroY, IndexedAbsoluteX, IndexedAbsoluteY, IndexedIndirectX, IndexedIndirectY,
	Implicit, Accumulator, Immediate, ZeroPage, Absolute, Relative, Indirect
};

enum Op
{
	ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS, CLC, CLD, CLI, CLV, CMP, CPX,
	CPY, DCP, DEC, DEX, DEY, EOR, INC, INX, INY, ISB, JMP, JSR, LAX, LDA, LDX, LDY, LSR, NOP, ORA,
	PHA, PHP, PLA, PLP, RLA, ROL, ROR, RRA, RTI, RTS, SAX, SBC, SEC, SED, SEI, SLO, SRE, STA, STX,
	STY, TAX, TAY, TSX, TXA, TXS, TYA, USBC
};

static const char OP_NAMES[][5] = {
		"ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRK", "BVC", "BVS", "CLC", "CLD", "CLI",
		"CLV", "CMP", "CPX", "CPY", "*DCP", "DEC", "DEX", "DEY", "EOR", "INC", "INX", "INY", "*ISB", "JMP", "JSR",
		"*LAX", "LDA", "LDX", "LDY", "LSR", "NOP", "ORA", "PHA", "PHP", "PLA", "PLP", "*RLA", "ROL", "ROR", "*RRA",
		"RTI", "RTS", "*SAX", "SBC", "SEC", "SED", "SEI", "*SLO", "*SRE", "STA", "STX", "STY", "TAX", "TAY", "TSX",
		"TXA", "TXS", "TYA", "*SBC"
};

typedef void (CPU::*func)();
struct opcode_info
{
	Op id;
	func op_func;
	ADDRESSING_MODE mode;
	u8 cycles;
};

struct registers_6502
{
	u8 acc, x, y, p, s;
	u16 pc;
};

enum STATUS
{
	c = 0x1, z = 0x2, i = 0x4, d = 0x8, b = 0x10, bit_5 = 0x20, v = 0x40, n = 0x80
};

enum CYCLE
{
	READ, WRITE, HALT
};

enum INTERRUPT_TYPE
{
	IRQ, NMI, BREAK
};


class CPU : public Processor
{
public:
	CPU();

	void reset() override;

	void init() override;

	bool run();

	void trigger_nmi()
	{
		PIN_NMI = true;
	};

	void trigger_irq()
	{
		PIN_IRQ = true;
	};

	void skip_cycles( int num, CYCLE type );

	u8 memory_regs[24];

protected:
	u8 read( int addr ) override;

	u8 read( int addr, bool physical_read );

	bool write( u16 addr, u8 data ) override;

private:
	void log_state();

	bool poll_interrupt();

	void interrupt( INTERRUPT_TYPE type );

	void exec( u8 opcode );

	void set_status( STATUS status, bool value );

	bool get_status( STATUS status ) const;

	void set_value_status( u8 val );

	void push_stack( u8 byte );

	u8 pop_stack();

	u8 peek_stack( u8 bytes );

	void branch( STATUS status, bool check_against );

	void compare( u8 a, u8 b );

	void push_address( u16 addr );

	u16 pop_address();

	u16 read_address( u16 addr );

	static u16 create_address( u8 lo, u8 hi );

	static bool same_page( u16 addr1, u16 addr2 );

	u8 shift_right( u8 byte );

	u8 shift_left( u8 byte );

	u8 rot_right( u8 byte );

	u8 rot_left( u8 byte );

	void add_with_carry( bool sub = false );

	void dummy_write( u16 addr, u8 data );

	//Opcode funcs
	void ADC(), AND(), ASL();

	void BCC(), BCS(), BEQ(), BIT(), BMI(), BNE(), BPL(), BRK(), BVC(), BVS();

	void CLC(), CLD(), CLI(), CLV(), CMP(), CPX(), CPY();

	void DCP(), DEC(), DEX(), DEY();

	void EOR();

	void INC(), INX(), INY(), ISB();

	void JMP(), JSR();

	void LAX(), LDA(), LDX(), LDY(), LSR();

	void NOP();

	void ORA();

	void PHA(), PHP(), PLA(), PLP();

	void RLA(), ROL(), ROR(), RRA(), RTI(), RTS();

	void SAX(), SBC(), SEC(), SED(), SEI(), SLO(), SRE(), STA(), STX(), STY();

	void TAX(), TAY(), TSX(), TXA(), TXS(), TYA();

	void USBC();

private:
	std::ostringstream oss;

	//Decode stage variables
	opcode_info curr_op;
	u8 ops[2];
	u16 addrs[2];
	i8 offset;
	bool inc_pc = true;

	bool oper_set = false;

	u8 oper();

	bool PIN_NMI = false;
	bool PIN_IRQ = false;
	int pending_interrupt = -1;

	bool polled_interrupt = false;
	bool suppress_skip_cycles = false;

	CYCLE state;

	inline constexpr static const u8 PAGE_SENSITIVE = 0xF0;
	inline static const std::unordered_map< u8, opcode_info > OPCODES = {
			{0x69, {Op::ADC, &CPU::ADC,  Immediate,        2}},
			{0x65, {Op::ADC, &CPU::ADC,  ZeroPage,         3}},
			{0x75, {Op::ADC, &CPU::ADC,  IndexedZeroX,     4}},
			{0x6D, {Op::ADC, &CPU::ADC,  Absolute,         4}},
			{0x7D, {Op::ADC, &CPU::ADC,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}},
			{0x79, {Op::ADC, &CPU::ADC,  IndexedAbsoluteY, 4 + PAGE_SENSITIVE}},
			{0x61, {Op::ADC, &CPU::ADC,  IndexedIndirectX, 6}},
			{0x71, {Op::ADC, &CPU::ADC,  IndexedIndirectY, 5 + PAGE_SENSITIVE}},
			{0x29, {Op::AND, &CPU::AND,  Immediate,        2}},
			{0x25, {Op::AND, &CPU::AND,  ZeroPage,         3}},
			{0x35, {Op::AND, &CPU::AND,  IndexedZeroX,     4}},
			{0x2D, {Op::AND, &CPU::AND,  Absolute,         4}},
			{0x3D, {Op::AND, &CPU::AND,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}},
			{0x39, {Op::AND, &CPU::AND,  IndexedAbsoluteY, 4 + PAGE_SENSITIVE}},
			{0x21, {Op::AND, &CPU::AND,  IndexedIndirectX, 6}},
			{0x31, {Op::AND, &CPU::AND,  IndexedIndirectY, 5 + PAGE_SENSITIVE}},
			{0x0A, {Op::ASL, &CPU::ASL,  Accumulator,      2}},
			{0x06, {Op::ASL, &CPU::ASL,  ZeroPage,         5}},
			{0x16, {Op::ASL, &CPU::ASL,  IndexedZeroX,     6}},
			{0x0E, {Op::ASL, &CPU::ASL,  Absolute,         6}},
			{0x1E, {Op::ASL, &CPU::ASL,  IndexedAbsoluteX, 7}},
			{0x90, {Op::BCC, &CPU::BCC,  Relative,         2}},
			{0xB0, {Op::BCS, &CPU::BCS,  Relative,         2}},
			{0xF0, {Op::BEQ, &CPU::BEQ,  Relative,         2}},
			{0x24, {Op::BIT, &CPU::BIT,  ZeroPage,         3}},
			{0x2C, {Op::BIT, &CPU::BIT,  Absolute,         4}},
			{0x30, {Op::BMI, &CPU::BMI,  Relative,         2}},
			{0xD0, {Op::BNE, &CPU::BNE,  Relative,         2}},
			{0x10, {Op::BPL, &CPU::BPL,  Relative,         2}},
			{0x00, {Op::BRK, &CPU::BRK,  Implicit,         7}},
			{0x50, {Op::BVC, &CPU::BVC,  Relative,         2}},
			{0x70, {Op::BVS, &CPU::BVS,  Relative,         2}},
			{0x18, {Op::CLC, &CPU::CLC,  Implicit,         2}},
			{0xD8, {Op::CLD, &CPU::CLD,  Implicit,         2}},
			{0x58, {Op::CLI, &CPU::CLI,  Implicit,         2}},
			{0xB8, {Op::CLV, &CPU::CLV,  Implicit,         2}},
			{0xC9, {Op::CMP, &CPU::CMP,  Immediate,        2}},
			{0xC5, {Op::CMP, &CPU::CMP,  ZeroPage,         3}},
			{0xD5, {Op::CMP, &CPU::CMP,  IndexedZeroX,     4}},
			{0xCD, {Op::CMP, &CPU::CMP,  Absolute,         4}},
			{0xDD, {Op::CMP, &CPU::CMP,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}},
			{0xD9, {Op::CMP, &CPU::CMP,  IndexedAbsoluteY, 4 + PAGE_SENSITIVE}},
			{0xC1, {Op::CMP, &CPU::CMP,  IndexedIndirectX, 6}},
			{0xD1, {Op::CMP, &CPU::CMP,  IndexedIndirectY, 5 + PAGE_SENSITIVE}},
			{0xE0, {Op::CPX, &CPU::CPX,  Immediate,        2}},
			{0xE4, {Op::CPX, &CPU::CPX,  ZeroPage,         3}},
			{0xEC, {Op::CPX, &CPU::CPX,  Absolute,         4}},
			{0xC0, {Op::CPY, &CPU::CPY,  Immediate,        2}},
			{0xC4, {Op::CPY, &CPU::CPY,  ZeroPage,         3}},
			{0xCC, {Op::CPY, &CPU::CPY,  Absolute,         4}},
			{0xC7, {Op::DCP, &CPU::DCP,  ZeroPage,         5}}, //*//
			{0xD7, {Op::DCP, &CPU::DCP,  IndexedZeroX,     6}}, //*//
			{0xCF, {Op::DCP, &CPU::DCP,  Absolute,         6}}, //*//
			{0xDF, {Op::DCP, &CPU::DCP,  IndexedAbsoluteX, 7}}, //*//
			{0xDB, {Op::DCP, &CPU::DCP,  IndexedAbsoluteY, 7}}, //*//
			{0xC3, {Op::DCP, &CPU::DCP,  IndexedIndirectX, 8}}, //*//
			{0xD3, {Op::DCP, &CPU::DCP,  IndexedIndirectY, 8}}, //*//
			{0xC6, {Op::DEC, &CPU::DEC,  ZeroPage,         5}},
			{0xD6, {Op::DEC, &CPU::DEC,  IndexedZeroX,     6}},
			{0xCE, {Op::DEC, &CPU::DEC,  Absolute,         6}},
			{0xDE, {Op::DEC, &CPU::DEC,  IndexedAbsoluteX, 7}},
			{0xCA, {Op::DEX, &CPU::DEX,  Implicit,         2}},
			{0x88, {Op::DEY, &CPU::DEY,  Implicit,         2}},
			{0x49, {Op::EOR, &CPU::EOR,  Immediate,        2}},
			{0x45, {Op::EOR, &CPU::EOR,  ZeroPage,         3}},
			{0x55, {Op::EOR, &CPU::EOR,  IndexedZeroX,     4}},
			{0x4D, {Op::EOR, &CPU::EOR,  Absolute,         4}},
			{0x5D, {Op::EOR, &CPU::EOR,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}},
			{0x59, {Op::EOR, &CPU::EOR,  IndexedAbsoluteY, 4 + PAGE_SENSITIVE}},
			{0x41, {Op::EOR, &CPU::EOR,  IndexedIndirectX, 6}},
			{0x51, {Op::EOR, &CPU::EOR,  IndexedIndirectY, 5 + PAGE_SENSITIVE}},
			{0xE6, {Op::INC, &CPU::INC,  ZeroPage,         5}},
			{0xF6, {Op::INC, &CPU::INC,  IndexedZeroX,     6}},
			{0xEE, {Op::INC, &CPU::INC,  Absolute,         6}},
			{0xFE, {Op::INC, &CPU::INC,  IndexedAbsoluteX, 7}},
			{0xE8, {Op::INX, &CPU::INX,  Implicit,         2}},
			{0xC8, {Op::INY, &CPU::INY,  Implicit,         2}},
			{0xE7, {Op::ISB, &CPU::ISB,  ZeroPage,         5}}, //*//
			{0xF7, {Op::ISB, &CPU::ISB,  IndexedZeroX,     6}}, //*//
			{0xEF, {Op::ISB, &CPU::ISB,  Absolute,         6}}, //*//
			{0xFF, {Op::ISB, &CPU::ISB,  IndexedAbsoluteX, 7}}, //*//
			{0xFB, {Op::ISB, &CPU::ISB,  IndexedAbsoluteY, 7}}, //*//
			{0xE3, {Op::ISB, &CPU::ISB,  IndexedIndirectX, 8}}, //*//
			{0xF3, {Op::ISB, &CPU::ISB,  IndexedIndirectY, 8}}, //*//
			{0x4C, {Op::JMP, &CPU::JMP,  Absolute,         3}},
			{0x6C, {Op::JMP, &CPU::JMP,  Indirect,         5}},
			{0x20, {Op::JSR, &CPU::JSR,  Absolute,         6}},
			{0xA7, {Op::LAX, &CPU::LAX,  ZeroPage,         3}}, //*//
			{0xB7, {Op::LAX, &CPU::LAX,  IndexedZeroY,     4}}, //*//
			{0xAF, {Op::LAX, &CPU::LAX,  Absolute,         4}}, //*//
			{0xBF, {Op::LAX, &CPU::LAX,  IndexedAbsoluteY, 4 + PAGE_SENSITIVE}}, //*//
			{0xA3, {Op::LAX, &CPU::LAX,  IndexedIndirectX, 6}}, //*//
			{0xB3, {Op::LAX, &CPU::LAX,  IndexedIndirectY, 5 + PAGE_SENSITIVE}}, //*//
			{0xA9, {Op::LDA, &CPU::LDA,  Immediate,        2}},
			{0xA5, {Op::LDA, &CPU::LDA,  ZeroPage,         3}},
			{0xB5, {Op::LDA, &CPU::LDA,  IndexedZeroX,     4}},
			{0xAD, {Op::LDA, &CPU::LDA,  Absolute,         4}},
			{0xBD, {Op::LDA, &CPU::LDA,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}},
			{0xB9, {Op::LDA, &CPU::LDA,  IndexedAbsoluteY, 4 + PAGE_SENSITIVE}},
			{0xA1, {Op::LDA, &CPU::LDA,  IndexedIndirectX, 6}},
			{0xB1, {Op::LDA, &CPU::LDA,  IndexedIndirectY, 5 + PAGE_SENSITIVE}},
			{0xA2, {Op::LDX, &CPU::LDX,  Immediate,        2}},
			{0xA6, {Op::LDX, &CPU::LDX,  ZeroPage,         3}},
			{0xB6, {Op::LDX, &CPU::LDX,  IndexedZeroY,     4}},
			{0xAE, {Op::LDX, &CPU::LDX,  Absolute,         4}},
			{0xBE, {Op::LDX, &CPU::LDX,  IndexedAbsoluteY, 4 + PAGE_SENSITIVE}},
			{0xA0, {Op::LDY, &CPU::LDY,  Immediate,        2}},
			{0xA4, {Op::LDY, &CPU::LDY,  ZeroPage,         3}},
			{0xB4, {Op::LDY, &CPU::LDY,  IndexedZeroX,     4}},
			{0xAC, {Op::LDY, &CPU::LDY,  Absolute,         4}},
			{0xBC, {Op::LDY, &CPU::LDY,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}},
			{0x4A, {Op::LSR, &CPU::LSR,  Accumulator,      2}},
			{0x46, {Op::LSR, &CPU::LSR,  ZeroPage,         5}},
			{0x56, {Op::LSR, &CPU::LSR,  IndexedZeroX,     6}},
			{0x4E, {Op::LSR, &CPU::LSR,  Absolute,         6}},
			{0x5E, {Op::LSR, &CPU::LSR,  IndexedAbsoluteX, 7}},
			{0xEA, {Op::NOP, &CPU::NOP,  Implicit,         2}},
			{0x09, {Op::ORA, &CPU::ORA,  Immediate,        2}},
			{0x05, {Op::ORA, &CPU::ORA,  ZeroPage,         3}},
			{0x15, {Op::ORA, &CPU::ORA,  IndexedZeroX,     4}},
			{0x0D, {Op::ORA, &CPU::ORA,  Absolute,         4}},
			{0x1D, {Op::ORA, &CPU::ORA,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}},
			{0x19, {Op::ORA, &CPU::ORA,  IndexedAbsoluteY, 4 + PAGE_SENSITIVE}},
			{0x01, {Op::ORA, &CPU::ORA,  IndexedIndirectX, 6}},
			{0x11, {Op::ORA, &CPU::ORA,  IndexedIndirectY, 5 + PAGE_SENSITIVE}},
			{0x48, {Op::PHA, &CPU::PHA,  Implicit,         3}},
			{0x08, {Op::PHP, &CPU::PHP,  Implicit,         3}},
			{0x68, {Op::PLA, &CPU::PLA,  Implicit,         4}},
			{0x28, {Op::PLP, &CPU::PLP,  Implicit,         4}},
			{0x27, {Op::RLA, &CPU::RLA,  ZeroPage,         5}}, //*//
			{0x37, {Op::RLA, &CPU::RLA,  IndexedZeroX,     6}}, //*//
			{0x2F, {Op::RLA, &CPU::RLA,  Absolute,         6}}, //*//
			{0x3F, {Op::RLA, &CPU::RLA,  IndexedAbsoluteX, 7}}, //*//
			{0x3B, {Op::RLA, &CPU::RLA,  IndexedAbsoluteY, 7}}, //*//
			{0x23, {Op::RLA, &CPU::RLA,  IndexedIndirectX, 8}}, //*//
			{0x33, {Op::RLA, &CPU::RLA,  IndexedIndirectY, 8}}, //*//
			{0x2A, {Op::ROL, &CPU::ROL,  Accumulator,      2}},
			{0x26, {Op::ROL, &CPU::ROL,  ZeroPage,         5}},
			{0x36, {Op::ROL, &CPU::ROL,  IndexedZeroX,     6}},
			{0x2E, {Op::ROL, &CPU::ROL,  Absolute,         6}},
			{0x3E, {Op::ROL, &CPU::ROL,  IndexedAbsoluteX, 7}},
			{0x6A, {Op::ROR, &CPU::ROR,  Accumulator,      2}},
			{0x66, {Op::ROR, &CPU::ROR,  ZeroPage,         5}},
			{0x76, {Op::ROR, &CPU::ROR,  IndexedZeroX,     6}},
			{0x6E, {Op::ROR, &CPU::ROR,  Absolute,         6}},
			{0x7E, {Op::ROR, &CPU::ROR,  IndexedAbsoluteX, 7}},
			{0x67, {Op::RRA, &CPU::RRA,  ZeroPage,         5}}, //*//
			{0x77, {Op::RRA, &CPU::RRA,  IndexedZeroX,     6}}, //*//
			{0x6F, {Op::RRA, &CPU::RRA,  Absolute,         6}}, //*//
			{0x7F, {Op::RRA, &CPU::RRA,  IndexedAbsoluteX, 7}}, //*//
			{0x7B, {Op::RRA, &CPU::RRA,  IndexedAbsoluteY, 7}}, //*//
			{0x63, {Op::RRA, &CPU::RRA,  IndexedIndirectX, 8}}, //*//
			{0x73, {Op::RRA, &CPU::RRA,  IndexedIndirectY, 8}}, //*//
			{0x40, {Op::RTI, &CPU::RTI,  Implicit,         6}},
			{0x60, {Op::RTS, &CPU::RTS,  Implicit,         6}},
			{0x87, {Op::SAX, &CPU::SAX,  ZeroPage,         3}}, //*//
			{0x97, {Op::SAX, &CPU::SAX,  IndexedZeroY,     4}}, //*//
			{0x8F, {Op::SAX, &CPU::SAX,  Absolute,         4}}, //*//
			{0x83, {Op::SAX, &CPU::SAX,  IndexedIndirectX, 6}}, //*//
			{0xE9, {Op::SBC, &CPU::SBC,  Immediate,        2}},
			{0xE5, {Op::SBC, &CPU::SBC,  ZeroPage,         3}},
			{0xF5, {Op::SBC, &CPU::SBC,  IndexedZeroX,     4}},
			{0xED, {Op::SBC, &CPU::SBC,  Absolute,         4}},
			{0xFD, {Op::SBC, &CPU::SBC,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}},
			{0xF9, {Op::SBC, &CPU::SBC,  IndexedAbsoluteY, 4 + PAGE_SENSITIVE}},
			{0xE1, {Op::SBC, &CPU::SBC,  IndexedIndirectX, 6}},
			{0xF1, {Op::SBC, &CPU::SBC,  IndexedIndirectY, 5 + PAGE_SENSITIVE}},
			{0x38, {Op::SEC, &CPU::SEC,  Implicit,         2}},
			{0xF8, {Op::SED, &CPU::SED,  Implicit,         2}},
			{0x78, {Op::SEI, &CPU::SEI,  Implicit,         2}},
			{0x07, {Op::SLO, &CPU::SLO,  ZeroPage,         5}}, //*//
			{0x17, {Op::SLO, &CPU::SLO,  IndexedZeroX,     6}}, //*//
			{0x0F, {Op::SLO, &CPU::SLO,  Absolute,         6}}, //*//
			{0x1F, {Op::SLO, &CPU::SLO,  IndexedAbsoluteX, 7}}, //*//
			{0x1B, {Op::SLO, &CPU::SLO,  IndexedAbsoluteY, 7}}, //*//
			{0x03, {Op::SLO, &CPU::SLO,  IndexedIndirectX, 8}}, //*//
			{0x13, {Op::SLO, &CPU::SLO,  IndexedIndirectY, 8}}, //*//
			{0x47, {Op::SRE, &CPU::SRE,  ZeroPage,         5}}, //*//
			{0x57, {Op::SRE, &CPU::SRE,  IndexedZeroX,     6}}, //*//
			{0x4F, {Op::SRE, &CPU::SRE,  Absolute,         6}}, //*//
			{0x5F, {Op::SRE, &CPU::SRE,  IndexedAbsoluteX, 7}}, //*//
			{0x5B, {Op::SRE, &CPU::SRE,  IndexedAbsoluteY, 7}}, //*//
			{0x43, {Op::SRE, &CPU::SRE,  IndexedIndirectX, 8}}, //*//
			{0x53, {Op::SRE, &CPU::SRE,  IndexedIndirectY, 8}}, //*//
			{0x85, {Op::STA, &CPU::STA,  ZeroPage,         3}},
			{0x95, {Op::STA, &CPU::STA,  IndexedZeroX,     4}},
			{0x8D, {Op::STA, &CPU::STA,  Absolute,         4}},
			{0x9D, {Op::STA, &CPU::STA,  IndexedAbsoluteX, 5}},
			{0x99, {Op::STA, &CPU::STA,  IndexedAbsoluteY, 5}},
			{0x81, {Op::STA, &CPU::STA,  IndexedIndirectX, 6}},
			{0x91, {Op::STA, &CPU::STA,  IndexedIndirectY, 6}},
			{0x86, {Op::STX, &CPU::STX,  ZeroPage,         3}},
			{0x96, {Op::STX, &CPU::STX,  IndexedZeroY,     4}},
			{0x8E, {Op::STX, &CPU::STX,  Absolute,         4}},
			{0x84, {Op::STY, &CPU::STY,  ZeroPage,         3}},
			{0x94, {Op::STY, &CPU::STY,  IndexedZeroX,     4}},
			{0x8C, {Op::STY, &CPU::STY,  Absolute,         4}},
			{0xAA, {Op::TAX, &CPU::TAX,  Implicit,         2}},
			{0xA8, {Op::TAY, &CPU::TAY,  Implicit,         2}},
			{0xBA, {Op::TSX, &CPU::TSX,  Implicit,         2}},
			{0x8A, {Op::TXA, &CPU::TXA,  Implicit,         2}},
			{0x9A, {Op::TXS, &CPU::TXS,  Implicit,         2}},
			{0x98, {Op::TYA, &CPU::TYA,  Implicit,         2}},
			{0xEB, {Op::USBC, &CPU::USBC, Immediate,        2}}, //*//
			// illegal NOPs
			{0x1A, {Op::NOP, &CPU::NOP,  Implicit,         2}}, //*//
			{0x3A, {Op::NOP, &CPU::NOP,  Implicit,         2}}, //*//
			{0x5A, {Op::NOP, &CPU::NOP,  Implicit,         2}}, //*//
			{0x7A, {Op::NOP, &CPU::NOP,  Implicit,         2}}, //*//
			{0xDA, {Op::NOP, &CPU::NOP,  Implicit,         2}}, //*//
			{0xFA, {Op::NOP, &CPU::NOP,  Implicit,         2}}, //*//
			{0x80, {Op::NOP, &CPU::NOP,  Immediate,        2}}, //*//
			{0x82, {Op::NOP, &CPU::NOP,  Immediate,        2}}, //*//
			{0x89, {Op::NOP, &CPU::NOP,  Immediate,        2}}, //*//
			{0xC2, {Op::NOP, &CPU::NOP,  Immediate,        2}}, //*//
			{0xE2, {Op::NOP, &CPU::NOP,  Immediate,        2}}, //*//
			{0x04, {Op::NOP, &CPU::NOP,  ZeroPage,         3}}, //*//
			{0x44, {Op::NOP, &CPU::NOP,  ZeroPage,         3}}, //*//
			{0x64, {Op::NOP, &CPU::NOP,  ZeroPage,         3}}, //*//
			{0x14, {Op::NOP, &CPU::NOP,  IndexedZeroX,     4}}, //*//
			{0x34, {Op::NOP, &CPU::NOP,  IndexedZeroX,     4}}, //*//
			{0x54, {Op::NOP, &CPU::NOP,  IndexedZeroX,     4}}, //*//
			{0x74, {Op::NOP, &CPU::NOP,  IndexedZeroX,     4}}, //*//
			{0xD4, {Op::NOP, &CPU::NOP,  IndexedZeroX,     4}}, //*//
			{0xF4, {Op::NOP, &CPU::NOP,  IndexedZeroX,     4}}, //*//
			{0x0C, {Op::NOP, &CPU::NOP,  Absolute,         4}}, //*//
			{0x1C, {Op::NOP, &CPU::NOP,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}}, //*//
			{0x3C, {Op::NOP, &CPU::NOP,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}}, //*//
			{0x5C, {Op::NOP, &CPU::NOP,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}}, //*//
			{0x7C, {Op::NOP, &CPU::NOP,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}}, //*//
			{0xDC, {Op::NOP, &CPU::NOP,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}}, //*//
			{0xFC, {Op::NOP, &CPU::NOP,  IndexedAbsoluteX, 4 + PAGE_SENSITIVE}}, //*//
	};;

	registers_6502 reg{};

	std::ostringstream regs_log;
	static constexpr bool logging = false;
};
