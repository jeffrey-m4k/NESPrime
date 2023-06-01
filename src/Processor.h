#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <iostream>
#include <cstdint>
#include <set>
#include <unordered_map>
#include "Memory.h"

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
    long cycle = 0;
    Memory mem;
    AddressSpace aspace;
};



class PPU : public Processor {
public:
    PPU();
    virtual void reset() override {};
    virtual void run() override {};
};


#endif //PROCESSOR_H
