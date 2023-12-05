#include "Memory.h"

#include <cstdint>

// TODO throw an error for an invalid address
uint8_t Memory::read(uint32_t addr) {
    return addr < size ? mem[addr] : 0x0;
}

bool Memory::write(const uint32_t addr, const uint8_t data) {
    return this->write(addr, &data, 1);
}

bool Memory::write(const uint32_t addr, const uint8_t* data, const int bytes) {
    if (addr + bytes <= size) {
        mem[addr] = *data;
        return true;
    }
    return false;
}

void Memory::init(const uint32_t size) {
    delete[] this->mem;
    this->size = size;
    this->mem = new uint8_t[size];
}


