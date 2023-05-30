#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <cstdint>
#include <set>
#include <unordered_map>
#include "Memory.h"
#include "Cartridge.h"

struct registers_6502 {
    uint8_t acc, x, y, p, s;
    uint16_t pc;
};

enum STATUS {
    c = 0x1, z = 0x2, i = 0x4, d = 0x8, b = 0x10, bit_5 = 0x20, v = 0x40, n = 0x80
};

enum ADDRESSING_MODE {
    IndexedZeroX, IndexedZeroY, IndexedAbsoluteX, IndexedAbsoluteY, IndexedIndirectX, IndexedIndirectY,
    Implicit, Accumulator, Immediate, ZeroPage, Absolute, Relative, Indirect
};

struct MappedBlock {
    uint16_t start_addr; // The starting mapped address
    uint8_t* start_mem; // The actual starting location in memory
    uint16_t size; // The size in bytes of the block
};

inline bool operator <(MappedBlock const& block, MappedBlock const& comp) {
    return block.start_addr < comp.start_addr;
}

class AddressSpace : public std::set<MappedBlock> {
public:
    uint8_t read(const int& addr);
    bool write(const uint16_t& addr, const uint8_t& data);
private:
    iterator get_block(const uint16_t& addr);
};

struct opcode_info {
    char id[4];
    ADDRESSING_MODE mode;
    uint8_t cycles;
};

class Processor {
public:
    Processor();
    ~Processor() {};
    virtual void reset() = 0;
    virtual void run() = 0;
protected:
    uint8_t read(const int& addr) { return aspace.read(addr); }
    bool write(const uint16_t& addr, const uint8_t& data) { return aspace.write(addr, data); }
protected:
    Memory mem;
    AddressSpace aspace;
};

class CPU : public Processor {
public:
    CPU();
    ~CPU() {};
    virtual void reset() override;
    virtual void run() override;
    bool map(const uint16_t& addr, uint8_t* block, const uint16_t& size);
    void print_registers();
    void print_stack();
private:
    void exec(const uint8_t& opcode);
    void set_status(const STATUS& status, bool value);
    bool get_status(const STATUS& status);
    void set_value_status(const uint8_t& val);
    void push_stack(const uint8_t& byte);
    uint8_t pop_stack();
    uint8_t peek_stack(const uint8_t& bytes);
    void branch(const STATUS& status, const bool& check_against, const uint8_t& offset);
    void compare(const uint8_t& a, const uint8_t& b);
    void adc(const uint8_t& arg);
    void pull_status();
    void push_address(const uint16_t& addr);
    uint16_t pop_address();
    uint16_t read_address(const uint16_t& addr);
    uint16_t create_address(const uint8_t& lo, const uint8_t& hi);
    bool same_page(const uint16_t& addr1, const uint16_t& addr2);
    uint8_t shift_right(uint8_t byte);
    uint8_t shift_left(uint8_t byte);
    uint8_t rot_right(uint8_t byte);
    uint8_t rot_left(uint8_t byte);
private:
    long cycle = 0;
    static const int CLOCK_SPEED = 1789773;
    constexpr static const double CPS = 1.0 / CLOCK_SPEED;
    static inline const std::unordered_map<uint8_t,opcode_info> OPCODES = {
            {0x78, {"SEI", Implicit, 2}},
            {0xD8, {"CLD", Implicit, 2}},
            {0xA9, {"LDA", Immediate, 2}},
            {0xA5, {"LDA", ZeroPage, 3}},
            {0xAD, {"LDA", Absolute, 4}},
            {0xA1, {"LDA", IndexedIndirectX, 6}},
            {0x85, {"STA", ZeroPage, 3}},
            {0x8D, {"STA", Absolute, 4}},
            {0x81, {"STA", IndexedIndirectX, 6}},
            {0x4C, {"JMP", Absolute, 3}},
            {0xA2, {"LDX", Immediate, 2}},
            {0xA6, {"LDX", ZeroPage, 3}},
            {0xAE, {"LDX", Absolute, 4}},
            {0xA0, {"LDY", Immediate, 2}},
            {0xA4, {"LDY", ZeroPage, 3}},
            {0xAC, {"LDY", Absolute, 4}},
            {0x20, {"JSR", Absolute, 6}},
            {0xEA, {"NOP", Implicit, 2}},
            {0x38, {"SEC", Implicit, 2}},
            {0xB0, {"BCS", Relative, 2}},
            {0x18, {"CLC", Implicit, 2}},
            {0x90, {"BCC", Relative, 2}},
            {0xF0, {"BEQ", Relative, 2}},
            {0xD0, {"BNE", Relative, 2}},
            {0x24, {"BIT", ZeroPage, 3}},
            {0x2C, {"BIT", Absolute, 4}},
            {0x70, {"BVS", Relative, 2}},
            {0x50, {"BVC", Relative, 2}},
            {0x10, {"BPL", Relative, 2}},
            {0x60, {"RTS", Implicit, 6}},
            {0xF8, {"SED", Implicit, 2}},
            {0x08, {"PHP", Implicit, 3}},
            {0x68, {"PLA", Implicit, 4}},
            {0x29, {"AND", Immediate, 2}},
            {0x25, {"AND", ZeroPage, 3}},
            {0x2D, {"AND", Absolute, 4}},
            {0x21, {"AND", IndexedIndirectX, 6}},
            {0xC9, {"CMP", Immediate, 2}},
            {0xC5, {"CMP", ZeroPage, 3}},
            {0xCD, {"CMP", Absolute, 4}},
            {0xC1, {"CMP", IndexedIndirectX, 6}},
            {0x48, {"PHA", Implicit, 3}},
            {0x28, {"PLP", Implicit, 4}},
            {0x30, {"BMI", Relative, 2}},
            {0x09, {"ORA", Immediate, 2}},
            {0x05, {"ORA", ZeroPage, 3}},
            {0x0D, {"ORA", Absolute, 4}},
            {0x01, {"ORA", IndexedIndirectX, 6}},
            {0xB8, {"CLV", Implicit, 2}},
            {0x49, {"EOR", Immediate, 2}},
            {0x45, {"EOR", ZeroPage, 3}},
            {0x4D, {"EOR", Absolute, 4}},
            {0x41, {"EOR", IndexedIndirectX, 6}},
            {0x69, {"ADC", Immediate, 2}},
            {0x65, {"ADC", ZeroPage, 3}},
            {0x6D, {"ADC", Absolute, 4}},
            {0x61, {"ADC", IndexedIndirectX, 6}},
            {0xE9, {"SBC", Immediate, 2}},
            {0xE5, {"SBC", ZeroPage, 3}},
            {0xED, {"SBC", Absolute, 4}},
            {0xE1, {"SBC", IndexedIndirectX, 6}},
            {0xC0, {"CPY", Immediate, 2}},
            {0xC4, {"CPY", ZeroPage, 3}},
            {0xCC, {"CPY", Absolute, 4}},
            {0xE0, {"CPX", Immediate, 2}},
            {0xE4, {"CPX", ZeroPage, 3}},
            {0xEC, {"CPX", Absolute, 4}},
            {0xE8, {"INX", Implicit, 2}},
            {0xC8, {"INY", Implicit, 2}},
            {0xCA, {"DEX", Implicit, 2}},
            {0x88, {"DEY", Implicit, 2}},
            {0xAA, {"TAX", Implicit, 2}},
            {0xA8, {"TAY", Implicit, 2}},
            {0x8A, {"TXA", Implicit, 2}},
            {0x98, {"TYA", Implicit, 2}},
            {0xBA, {"TSX", Implicit, 2}},
            {0x9A, {"TXS", Implicit, 2}},
            {0x86, {"STX", ZeroPage, 3}},
            {0x8E, {"STX", Absolute, 4}},
            {0x84, {"STY", ZeroPage, 3}},
            {0x8C, {"STY", Absolute, 4}},
            {0x40, {"RTI", Implicit, 6}},
            {0x4A, {"LSR", Accumulator, 2}},
            {0x46, {"LSR", ZeroPage, 5}},
            {0x4E, {"LSR", Absolute, 6}},
            {0x0A, {"ASL", Accumulator, 2}},
            {0x06, {"ASL", ZeroPage, 5}},
            {0x0E, {"ASL", Absolute, 6}},
            {0x6A, {"ROR", Accumulator, 2}},
            {0x66, {"ROR", ZeroPage, 5}},
            {0x6E, {"ROR", Absolute, 6}},
            {0x2A, {"ROL", Accumulator, 2}},
            {0x26, {"ROL", ZeroPage, 5}},
            {0x2E, {"ROL", Absolute, 6}},
            {0xE6, {"INC", ZeroPage, 5}},
            {0xEE, {"INC", Absolute, 6}},
            {0xC6, {"DEC", ZeroPage, 5}},
            {0xCE, {"DEC", Absolute, 6}}
    };;

    registers_6502 reg{};
    uint8_t stack[256]{};

};

class PPU : public Processor {
public:
    PPU();
    virtual void reset() override {};
    virtual void run() override {};
};


#endif //PROCESSOR_H
