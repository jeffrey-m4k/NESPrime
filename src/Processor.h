#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <cstdint>
#include <set>
#include "Memory.h"
#include "Cartridge.h"

struct registers_6502 {
    uint8_t acc, x, y, p, s;
    uint16_t pc;
};

struct MappedBlock {
    uint16_t start_addr; // The starting mapped address
    uint8_t* start_mem; // The actual starting location in memory
    uint16_t size; // The size in bytes of the block
};

inline bool operator <(MappedBlock const& block, MappedBlock const& comp) {
    return block.start_addr < comp.start_addr;
}

typedef std::set<MappedBlock> address_space;

class Processor {
public:
    Processor();
    ~Processor() {};
    virtual void reset() = 0;
    virtual void run() = 0;
protected:
    Memory mem;
    address_space aspace;
};

class CPU : public Processor {
public:
    CPU();
    ~CPU() {};
    virtual void reset() override;
    virtual void run() override;
    bool map(const uint16_t &addr, uint8_t *block, const uint16_t &size);
    void print_registers();
    void print_stack();
private:
    uint8_t read(const uint16_t& addr);
private:
    static const int CLOCK_SPEED = 1789773;
    constexpr static const double CPS = 1.0 / CLOCK_SPEED;

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
