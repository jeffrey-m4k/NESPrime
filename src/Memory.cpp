#include "Memory.h"

void Memory::init(const uint32_t size) {
    delete[] this->mem;
    this->size = size;
    this->mem = new uint8_t[size];
}


