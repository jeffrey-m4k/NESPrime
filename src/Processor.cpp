#include <algorithm>
#include "Processor.h"

Processor::Processor() {
    this->mem.init(0x800);
}

CPU::CPU() : Processor() {
    this->CPU::reset();
}

void CPU::reset() {
    // Clear the address space and map the CPU's 2KB memory from 0x0000 to 0x2000
    this->aspace.clear();
    for (int i = 0; i < 4; i++) {
        this->aspace.insert(MappedBlock{static_cast<uint16_t>(i*0x800), this->mem.get_mem(), 0x800});
    }
}

void CPU::run() {
    for(;;) {

    }
}

uint8_t CPU::read(const uint16_t &addr) {
    if (addr < 0x2000) {
        return mem.read(addr % 0x800);
    }
    else if (addr < 0x4000) {
        return mem.read(0x2000 + addr % 8);
    }
    else if (addr < 0x4018) {
        return mem.read(addr);
    }
    else if (addr < 0x4020) {
        return 0;
    }
    else {
        return mem.read(addr);
    }
}

bool CPU::map(const uint16_t &addr, uint8_t *block, const uint16_t &size) {
    // Validate that the mapped block would not intersect another before mapping
    auto mb = MappedBlock{addr, block, size};
    auto it = aspace.lower_bound(mb);
    if (it != aspace.begin() && (--it)->start_addr + it->size >= addr) return false;

    aspace.insert(mb);
    return true;
}
