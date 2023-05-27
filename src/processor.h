#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <cstdint>
#include "memory.h"

struct registers {
    uint8_t acc, x, y, p, s;
    uint16_t pc;
};

class processor {
public:
    processor() : mem(nullptr) {};
    processor(memory& mem) : mem(&mem) {};
    ~processor() {};
    void reset();
    void run();
    void print_registers();
    void print_stack();
private:
    static const int CLOCK_SPEED = 1789773;
    constexpr static const double CPS = 1.0 / CLOCK_SPEED;

    registers reg{};
    unsigned char stack[256]{};
    memory* mem;
};


#endif //PROCESSOR_H
