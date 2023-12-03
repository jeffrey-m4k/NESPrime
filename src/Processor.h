#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <iostream>
#include <cstdint>
#include <set>
#include <unordered_map>
#include "Memory.h"
#include "Bus.h"
#include "Component.h"

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
    uint8_t read(int addr);
    bool write(uint16_t addr, uint8_t data);
protected:
    iterator get_block(uint16_t addr);
};

class Processor : public Component {
public:
    Processor();
    ~Processor() { delete mem.get_mem(); };
    virtual void reset() = 0;
    virtual void init() = 0;
    virtual bool run();

    bool map(uint16_t addr, uint8_t* block, uint16_t size);

    long get_cycle() { return cycle; }
protected:
    virtual uint8_t read(const int addr) { return aspace.read(addr); }
    virtual bool write(const uint16_t addr, const uint8_t data) { return aspace.write(addr, data); }
protected:
    int idle_cycles = 0;
    long cycle = 0;
    Memory mem;
    AddressSpace aspace;
};

#endif //PROCESSOR_H
