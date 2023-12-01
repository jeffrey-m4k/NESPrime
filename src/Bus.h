#ifndef BUS_H
#define BUS_H

#include <cstdint>

class Bus {
private:
    uint16_t address;
    uint8_t data;
    bool control;
};


#endif
