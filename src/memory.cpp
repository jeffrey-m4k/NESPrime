#include "memory.h"

const unsigned char memory::read(const unsigned short &addr) {
    if (addr < 0x2000) {
        return mem[addr % 0x800];
    }
    else if (addr < 0x4000) {
        return mem[0x2000 + addr % 8];
    }
    else if (addr < 0x4018) {
        return mem[addr];
    }
    else if (addr < 0x4020) {
        return 0;
    }
    else {
        return mem[addr];
    }
}

void memory::write(const unsigned short &addr, const unsigned char &data) {
    mem[addr] = data;
}
