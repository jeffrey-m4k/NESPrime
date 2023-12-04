#include <algorithm>
#include <iostream>
#include "Processor.h"

Processor::Processor() {
    this->mem.init(0x800);
}

bool Processor::run() {
    if (idle_cycles > 0) {
        cycle++;
        idle_cycles--;
        return false;
    }

    idle_cycles = 0;
    return true;
}
