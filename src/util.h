#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <iomanip>

inline void print_hex(uint8_t* buffer, const uint16_t& size) {
    std::cout.fill('0');
    for (int row = 0; row < size / 16; row++) {
        for (int col = 0; col < 16; col++) {
            std::cout << std::setw(2) << std::hex << (int)buffer[row * 16 + col] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;
}

#endif //UTIL_H
