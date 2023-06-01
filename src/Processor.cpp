#include <algorithm>
#include <iostream>
#include "Processor.h"

AddressSpace::iterator AddressSpace::get_block(const uint16_t &addr) {
    auto dummy = MappedBlock{addr, 0, 0};
    auto it = lower_bound(dummy);
    if (it->start_addr == addr || (--it)->start_addr + it->size > addr)
        return it;
    else
        return this->end();
}

uint8_t AddressSpace::read(const int& addr) {
    auto it = get_block(addr);
    if (it != this->end())
        return it->start_mem[addr-it->start_addr];
    else
        return 0;
}

bool AddressSpace::write(const uint16_t& addr, const uint8_t& data) {
    auto it = get_block(addr);
    if (it != this->end()) {
        it->start_mem[addr-it->start_addr] = data;
        return true;
    } else
        return false;
}

Processor::Processor() {
    this->mem.init(0x800);
}




