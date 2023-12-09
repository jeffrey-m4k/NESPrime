#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <sstream>
#include "Processor.h"
#include "mappers/Mapper.h"

enum ADDRESSING_MODE {
    IndexedZeroX, IndexedZeroY, IndexedAbsoluteX, IndexedAbsoluteY, IndexedIndirectX, IndexedIndirectY,
    Implicit, Accumulator, Immediate, ZeroPage, Absolute, Relative, Indirect
};

enum Op {
    ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS, CLC, CLD, CLI, CLV, CMP, CPX,
    CPY, DCP, DEC, DEX, DEY, EOR, INC, INX, INY, ISB, JMP, JSR, LAX, LDA, LDX, LDY, LSR, NOP, ORA,
    PHA, PHP, PLA, PLP, RLA, ROL, ROR, RRA, RTI, RTS, SAX, SBC, SEC, SED, SEI, SLO, SRE, STA, STX,
    STY, TAX, TAY, TSX, TXA, TXS, TYA, USBC
};

static const char OP_NAMES[][5] = {
        "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRK", "BVC", "BVS", "CLC", "CLD", "CLI", "CLV", "CMP", "CPX",
        "CPY", "*DCP", "DEC", "DEX", "DEY", "EOR", "INC", "INX", "INY", "*ISB", "JMP", "JSR", "*LAX", "LDA", "LDX", "LDY", "LSR", "NOP", "ORA",
        "PHA", "PHP", "PLA", "PLP", "*RLA", "ROL", "ROR", "*RRA", "RTI", "RTS", "*SAX", "SBC", "SEC", "SED", "SEI", "*SLO", "*SRE", "STA", "STX",
        "STY", "TAX", "TAY", "TSX", "TXA", "TXS", "TYA", "*SBC"
};

struct opcode_info {
    Op id;
    ADDRESSING_MODE mode;
    uint8_t cycles;
};

struct registers_6502 {
    uint8_t acc, x, y, p, s;
    uint16_t pc;
};

enum STATUS {
    c = 0x1, z = 0x2, i = 0x4, d = 0x8, b = 0x10, bit_5 = 0x20, v = 0x40, n = 0x80
};

enum CYCLE {
    READ, WRITE, HALT
};

enum INTERRUPT_TYPE {
    IRQ, NMI, BREAK
};

class CPU : public Processor {
public:
    CPU();
    void reset() override;
    void init() override;
    bool run();
    void trigger_nmi() { PIN_NMI = true; };
    void trigger_irq() { PIN_IRQ = true; };

    uint8_t memory_regs[24];
protected:
    uint8_t read(int addr) override;
    uint8_t read(int addr, bool physical_read);
    bool write(uint16_t addr, uint8_t data) override;
private:
    void interrupt(INTERRUPT_TYPE type);
    void skip_cycles(int num, CYCLE type);
    void exec(uint8_t opcode);
    void set_status(STATUS status, bool value);
    bool get_status(STATUS status) const;
    void set_value_status(uint8_t val);
    void push_stack(uint8_t byte);
    uint8_t pop_stack();
    uint8_t peek_stack(uint8_t bytes);
    void branch(STATUS status, bool check_against);
    void compare(uint8_t a, uint8_t b);
    void push_address(uint16_t addr);
    uint16_t pop_address();
    uint16_t read_address(uint16_t addr);
    static uint16_t create_address(uint8_t lo, uint8_t hi);
    static bool same_page(uint16_t addr1, uint16_t addr2);

    uint8_t shift_right(uint8_t byte);
    uint8_t shift_left(uint8_t byte);
    uint8_t rot_right(uint8_t byte);
    uint8_t rot_left(uint8_t byte);
    void add_with_carry(bool sub = false);

    void dummy_write(uint16_t addr, uint8_t data);

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
    uint8_t ops[2];
    uint16_t addrs[2];
    int8_t offset;
    bool inc_pc = true;

    bool oper_set = false;
    uint8_t oper();

    bool PIN_NMI = false;
    bool PIN_IRQ = false;

    CYCLE state;

    inline constexpr static const uint8_t PAGE_SENSITIVE = 0xF0;
    inline static const std::unordered_map<uint8_t,opcode_info> OPCODES = {
            {0x69, {Op::ADC, Immediate, 2}},
            {0x65, {Op::ADC, ZeroPage, 3}},
            {0x75, {Op::ADC, IndexedZeroX, 4}},
            {0x6D, {Op::ADC, Absolute, 4}},
            {0x7D, {Op::ADC, IndexedAbsoluteX, 4+PAGE_SENSITIVE}},
            {0x79, {Op::ADC, IndexedAbsoluteY, 4+PAGE_SENSITIVE}},
            {0x61, {Op::ADC, IndexedIndirectX, 6}},
            {0x71, {Op::ADC, IndexedIndirectY, 5+PAGE_SENSITIVE}},
            {0x29, {Op::AND, Immediate, 2}},
            {0x25, {Op::AND, ZeroPage, 3}},
            {0x35, {Op::AND, IndexedZeroX, 4}},
            {0x2D, {Op::AND, Absolute, 4}},
            {0x3D, {Op::AND, IndexedAbsoluteX, 4+PAGE_SENSITIVE}},
            {0x39, {Op::AND, IndexedAbsoluteY, 4+PAGE_SENSITIVE}},
            {0x21, {Op::AND, IndexedIndirectX, 6}},
            {0x31, {Op::AND, IndexedIndirectY, 5+PAGE_SENSITIVE}},
            {0x0A, {Op::ASL, Accumulator, 2}},
            {0x06, {Op::ASL, ZeroPage, 5}},
            {0x16, {Op::ASL, IndexedZeroX, 6}},
            {0x0E, {Op::ASL, Absolute, 6}},
            {0x1E, {Op::ASL, IndexedAbsoluteX, 7}},
            {0x90, {Op::BCC, Relative, 2}},
            {0xB0, {Op::BCS, Relative, 2}},
            {0xF0, {Op::BEQ, Relative, 2}},
            {0x24, {Op::BIT, ZeroPage, 3}},
            {0x2C, {Op::BIT, Absolute, 4}},
            {0x30, {Op::BMI, Relative, 2}},
            {0xD0, {Op::BNE, Relative, 2}},
            {0x10, {Op::BPL, Relative, 2}},
            {0x00, {Op::BRK, Implicit, 7}},
            {0x50, {Op::BVC, Relative, 2}},
            {0x70, {Op::BVS, Relative, 2}},
            {0x18, {Op::CLC, Implicit, 2}},
            {0xD8, {Op::CLD, Implicit, 2}},
            {0x58, {Op::CLI, Implicit, 2}},
            {0xB8, {Op::CLV, Implicit, 2}},
            {0xC9, {Op::CMP, Immediate, 2}},
            {0xC5, {Op::CMP, ZeroPage, 3}},
            {0xD5, {Op::CMP, IndexedZeroX, 4}},
            {0xCD, {Op::CMP, Absolute, 4}},
            {0xDD, {Op::CMP, IndexedAbsoluteX, 4+PAGE_SENSITIVE}},
            {0xD9, {Op::CMP, IndexedAbsoluteY, 4+PAGE_SENSITIVE}},
            {0xC1, {Op::CMP, IndexedIndirectX, 6}},
            {0xD1, {Op::CMP, IndexedIndirectY, 5+PAGE_SENSITIVE}},
            {0xE0, {Op::CPX, Immediate, 2}},
            {0xE4, {Op::CPX, ZeroPage, 3}},
            {0xEC, {Op::CPX, Absolute, 4}},
            {0xC0, {Op::CPY, Immediate, 2}},
            {0xC4, {Op::CPY, ZeroPage, 3}},
            {0xCC, {Op::CPY, Absolute, 4}},
            {0xC7, {Op::DCP, ZeroPage, 5}}, //*//
            {0xD7, {Op::DCP, IndexedZeroX, 6}}, //*//
            {0xCF, {Op::DCP, Absolute, 6}}, //*//
            {0xDF, {Op::DCP, IndexedAbsoluteX, 7}}, //*//
            {0xDB, {Op::DCP, IndexedAbsoluteY, 7}}, //*//
            {0xC3, {Op::DCP, IndexedIndirectX, 8}}, //*//
            {0xD3, {Op::DCP, IndexedIndirectY, 8}}, //*//
            {0xC6, {Op::DEC, ZeroPage, 5}},
            {0xD6, {Op::DEC, IndexedZeroX, 6}},
            {0xCE, {Op::DEC, Absolute, 6}},
            {0xDE, {Op::DEC, IndexedAbsoluteX, 7}},
            {0xCA, {Op::DEX, Implicit, 2}},
            {0x88, {Op::DEY, Implicit, 2}},
            {0x49, {Op::EOR, Immediate, 2}},
            {0x45, {Op::EOR, ZeroPage, 3}},
            {0x55, {Op::EOR, IndexedZeroX, 4}},
            {0x4D, {Op::EOR, Absolute, 4}},
            {0x5D, {Op::EOR, IndexedAbsoluteX, 4+PAGE_SENSITIVE}},
            {0x59, {Op::EOR, IndexedAbsoluteY, 4+PAGE_SENSITIVE}},
            {0x41, {Op::EOR, IndexedIndirectX, 6}},
            {0x51, {Op::EOR, IndexedIndirectY, 5+PAGE_SENSITIVE}},
            {0xE6, {Op::INC, ZeroPage, 5}},
            {0xF6, {Op::INC, IndexedZeroX, 6}},
            {0xEE, {Op::INC, Absolute, 6}},
            {0xFE, {Op::INC, IndexedAbsoluteX, 7}},
            {0xE8, {Op::INX, Implicit, 2}},
            {0xC8, {Op::INY, Implicit, 2}},
            {0xE7, {Op::ISB, ZeroPage, 5}}, //*//
            {0xF7, {Op::ISB, IndexedZeroX, 6}}, //*//
            {0xEF, {Op::ISB, Absolute, 6}}, //*//
            {0xFF, {Op::ISB, IndexedAbsoluteX, 7}}, //*//
            {0xFB, {Op::ISB, IndexedAbsoluteY, 7}}, //*//
            {0xE3, {Op::ISB, IndexedIndirectX, 8}}, //*//
            {0xF3, {Op::ISB, IndexedIndirectY, 8}}, //*//
            {0x4C, {Op::JMP, Absolute, 3}},
            {0x6C, {Op::JMP, Indirect, 5}},
            {0x20, {Op::JSR, Absolute, 6}},
            {0xA7, {Op::LAX, ZeroPage, 3}}, //*//
            {0xB7, {Op::LAX, IndexedZeroY, 4}}, //*//
            {0xAF, {Op::LAX, Absolute, 4}}, //*//
            {0xBF, {Op::LAX, IndexedAbsoluteY, 4+PAGE_SENSITIVE}}, //*//
            {0xA3, {Op::LAX, IndexedIndirectX, 6}}, //*//
            {0xB3, {Op::LAX, IndexedIndirectY, 5+PAGE_SENSITIVE}}, //*//
            {0xA9, {Op::LDA, Immediate, 2}},
            {0xA5, {Op::LDA, ZeroPage, 3}},
            {0xB5, {Op::LDA, IndexedZeroX, 4}},
            {0xAD, {Op::LDA, Absolute, 4}},
            {0xBD, {Op::LDA, IndexedAbsoluteX, 4+PAGE_SENSITIVE}},
            {0xB9, {Op::LDA, IndexedAbsoluteY, 4+PAGE_SENSITIVE}},
            {0xA1, {Op::LDA, IndexedIndirectX, 6}},
            {0xB1, {Op::LDA, IndexedIndirectY, 5+PAGE_SENSITIVE}},
            {0xA2, {Op::LDX, Immediate, 2}},
            {0xA6, {Op::LDX, ZeroPage, 3}},
            {0xB6, {Op::LDX, IndexedZeroY, 4}},
            {0xAE, {Op::LDX, Absolute, 4}},
            {0xBE, {Op::LDX, IndexedAbsoluteY, 4+PAGE_SENSITIVE}},
            {0xA0, {Op::LDY, Immediate, 2}},
            {0xA4, {Op::LDY, ZeroPage, 3}},
            {0xB4, {Op::LDY, IndexedZeroX, 4}},
            {0xAC, {Op::LDY, Absolute, 4}},
            {0xBC, {Op::LDY, IndexedAbsoluteX, 4+PAGE_SENSITIVE}},
            {0x4A, {Op::LSR, Accumulator, 2}},
            {0x46, {Op::LSR, ZeroPage, 5}},
            {0x56, {Op::LSR, IndexedZeroX, 6}},
            {0x4E, {Op::LSR, Absolute, 6}},
            {0x5E, {Op::LSR, IndexedAbsoluteX, 7}},
            {0xEA, {Op::NOP, Implicit, 2}},
            {0x09, {Op::ORA, Immediate, 2}},
            {0x05, {Op::ORA, ZeroPage, 3}},
            {0x15, {Op::ORA, IndexedZeroX, 4}},
            {0x0D, {Op::ORA, Absolute, 4}},
            {0x1D, {Op::ORA, IndexedAbsoluteX, 4+PAGE_SENSITIVE}},
            {0x19, {Op::ORA, IndexedAbsoluteY, 4+PAGE_SENSITIVE}},
            {0x01, {Op::ORA, IndexedIndirectX, 6}},
            {0x11, {Op::ORA, IndexedIndirectY, 5+PAGE_SENSITIVE}},
            {0x48, {Op::PHA, Implicit, 3}},
            {0x08, {Op::PHP, Implicit, 3}},
            {0x68, {Op::PLA, Implicit, 4}},
            {0x28, {Op::PLP, Implicit, 4}},
            {0x27, {Op::RLA, ZeroPage, 5}}, //*//
            {0x37, {Op::RLA, IndexedZeroX, 6}}, //*//
            {0x2F, {Op::RLA, Absolute, 6}}, //*//
            {0x3F, {Op::RLA, IndexedAbsoluteX, 7}}, //*//
            {0x3B, {Op::RLA, IndexedAbsoluteY, 7}}, //*//
            {0x23, {Op::RLA, IndexedIndirectX, 8}}, //*//
            {0x33, {Op::RLA, IndexedIndirectY, 8}}, //*//
            {0x2A, {Op::ROL, Accumulator, 2}},
            {0x26, {Op::ROL, ZeroPage, 5}},
            {0x36, {Op::ROL, IndexedZeroX, 6}},
            {0x2E, {Op::ROL, Absolute, 6}},
            {0x3E, {Op::ROL, IndexedAbsoluteX, 7}},
            {0x6A, {Op::ROR, Accumulator, 2}},
            {0x66, {Op::ROR, ZeroPage, 5}},
            {0x76, {Op::ROR, IndexedZeroX, 6}},
            {0x6E, {Op::ROR, Absolute, 6}},
            {0x7E, {Op::ROR, IndexedAbsoluteX, 7}},
            {0x67, {Op::RRA, ZeroPage, 5}}, //*//
            {0x77, {Op::RRA, IndexedZeroX, 6}}, //*//
            {0x6F, {Op::RRA, Absolute, 6}}, //*//
            {0x7F, {Op::RRA, IndexedAbsoluteX, 7}}, //*//
            {0x7B, {Op::RRA, IndexedAbsoluteY, 7}}, //*//
            {0x63, {Op::RRA, IndexedIndirectX, 8}}, //*//
            {0x73, {Op::RRA, IndexedIndirectY, 8}}, //*//
            {0x40, {Op::RTI, Implicit, 6}},
            {0x60, {Op::RTS, Implicit, 6}},
            {0x87, {Op::SAX, ZeroPage, 3}}, //*//
            {0x97, {Op::SAX, IndexedZeroY, 4}}, //*//
            {0x8F, {Op::SAX, Absolute, 4}}, //*//
            {0x83, {Op::SAX, IndexedIndirectX, 6}}, //*//
            {0xE9, {Op::SBC, Immediate, 2}},
            {0xE5, {Op::SBC, ZeroPage, 3}},
            {0xF5, {Op::SBC, IndexedZeroX, 4}},
            {0xED, {Op::SBC, Absolute, 4}},
            {0xFD, {Op::SBC, IndexedAbsoluteX, 4+PAGE_SENSITIVE}},
            {0xF9, {Op::SBC, IndexedAbsoluteY, 4+PAGE_SENSITIVE}},
            {0xE1, {Op::SBC, IndexedIndirectX, 6}},
            {0xF1, {Op::SBC, IndexedIndirectY, 5+PAGE_SENSITIVE}},
            {0x38, {Op::SEC, Implicit, 2}},
            {0xF8, {Op::SED, Implicit, 2}},
            {0x78, {Op::SEI, Implicit, 2}},
            {0x07, {Op::SLO, ZeroPage, 5}}, //*//
            {0x17, {Op::SLO, IndexedZeroX, 6}}, //*//
            {0x0F, {Op::SLO, Absolute, 6}}, //*//
            {0x1F, {Op::SLO, IndexedAbsoluteX, 7}}, //*//
            {0x1B, {Op::SLO, IndexedAbsoluteY, 7}}, //*//
            {0x03, {Op::SLO, IndexedIndirectX, 8}}, //*//
            {0x13, {Op::SLO, IndexedIndirectY, 8}}, //*//
            {0x47, {Op::SRE, ZeroPage, 5}}, //*//
            {0x57, {Op::SRE, IndexedZeroX, 6}}, //*//
            {0x4F, {Op::SRE, Absolute, 6}}, //*//
            {0x5F, {Op::SRE, IndexedAbsoluteX, 7}}, //*//
            {0x5B, {Op::SRE, IndexedAbsoluteY, 7}}, //*//
            {0x43, {Op::SRE, IndexedIndirectX, 8}}, //*//
            {0x53, {Op::SRE, IndexedIndirectY, 8}}, //*//
            {0x85, {Op::STA, ZeroPage, 3}},
            {0x95, {Op::STA, IndexedZeroX, 4}},
            {0x8D, {Op::STA, Absolute, 4}},
            {0x9D, {Op::STA, IndexedAbsoluteX, 5}},
            {0x99, {Op::STA, IndexedAbsoluteY, 5}},
            {0x81, {Op::STA, IndexedIndirectX, 6}},
            {0x91, {Op::STA, IndexedIndirectY, 6}},
            {0x86, {Op::STX, ZeroPage, 3}},
            {0x96, {Op::STX, IndexedZeroY, 4}},
            {0x8E, {Op::STX, Absolute, 4}},
            {0x84, {Op::STY, ZeroPage, 3}},
            {0x94, {Op::STY, IndexedZeroX, 4}},
            {0x8C, {Op::STY, Absolute, 4}},
            {0xAA, {Op::TAX, Implicit, 2}},
            {0xA8, {Op::TAY, Implicit, 2}},
            {0xBA, {Op::TSX, Implicit, 2}},
            {0x8A, {Op::TXA, Implicit, 2}},
            {0x9A, {Op::TXS, Implicit, 2}},
            {0x98, {Op::TYA, Implicit, 2}},
            {0xEB, {Op::USBC, Immediate, 2}}, //*//
            // illegal NOPs
            {0x1A, {Op::NOP, Implicit, 2}}, //*//
            {0x3A, {Op::NOP, Implicit, 2}}, //*//
            {0x5A, {Op::NOP, Implicit, 2}}, //*//
            {0x7A, {Op::NOP, Implicit, 2}}, //*//
            {0xDA, {Op::NOP, Implicit, 2}}, //*//
            {0xFA, {Op::NOP, Implicit, 2}}, //*//
            {0x80, {Op::NOP, Immediate, 2}}, //*//
            {0x82, {Op::NOP, Immediate, 2}}, //*//
            {0x89, {Op::NOP, Immediate, 2}}, //*//
            {0xC2, {Op::NOP, Immediate, 2}}, //*//
            {0xE2, {Op::NOP, Immediate, 2}}, //*//
            {0x04, {Op::NOP, ZeroPage, 3}}, //*//
            {0x44, {Op::NOP, ZeroPage, 3}}, //*//
            {0x64, {Op::NOP, ZeroPage, 3}}, //*//
            {0x14, {Op::NOP, IndexedZeroX, 4}}, //*//
            {0x34, {Op::NOP, IndexedZeroX, 4}}, //*//
            {0x54, {Op::NOP, IndexedZeroX, 4}}, //*//
            {0x74, {Op::NOP, IndexedZeroX, 4}}, //*//
            {0xD4, {Op::NOP, IndexedZeroX, 4}}, //*//
            {0xF4, {Op::NOP, IndexedZeroX, 4}}, //*//
            {0x0C, {Op::NOP, Absolute, 4}}, //*//
            {0x1C, {Op::NOP, IndexedAbsoluteX, 4+PAGE_SENSITIVE}}, //*//
            {0x3C, {Op::NOP, IndexedAbsoluteX, 4+PAGE_SENSITIVE}}, //*//
            {0x5C, {Op::NOP, IndexedAbsoluteX, 4+PAGE_SENSITIVE}}, //*//
            {0x7C, {Op::NOP, IndexedAbsoluteX, 4+PAGE_SENSITIVE}}, //*//
            {0xDC, {Op::NOP, IndexedAbsoluteX, 4+PAGE_SENSITIVE}}, //*//
            {0xFC, {Op::NOP, IndexedAbsoluteX, 4+PAGE_SENSITIVE}}, //*//
    };;

    registers_6502 reg{};

    std::ostringstream regs_log;
    bool logging = false;
};


#endif //CPU_H
