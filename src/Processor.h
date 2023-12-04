#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <iostream>
#include <cstdint>
#include <set>
#include <unordered_map>
#include "Memory.h"
#include "Bus.h"
#include "Component.h"

class Mapper;

class Processor : public Component {
public:
    Processor();
    ~Processor() { delete[] mem.get_mem(); };
    virtual void reset() = 0;
    virtual void init() = 0;
    virtual bool run();

    bool map(uint16_t addr, uint8_t* block, uint16_t size);

    long get_cycle() { return cycle; }
    Memory* get_mem() { return &mem; }
    void set_mapper(Mapper* mapperIn) { mapper = mapperIn; }
protected:
    virtual uint8_t read(int addr) = 0;
    virtual bool write(uint16_t addr, uint8_t data) = 0;
protected:
    int idle_cycles = 0;
    long cycle = 0;
    Memory mem;
    Mapper* mapper;
};

#endif //PROCESSOR_H
