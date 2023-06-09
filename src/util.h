#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <iomanip>
using std::hex, std::dec, std::uppercase, std::setw, std::cout, std::endl;

inline void print_hex(std::ostream& out, const uint16_t& num, const std::string& append = "") {
    std::ios_base::fmtflags f(cout.flags());
    if (num < 0x10) out << "0";
    out << hex << uppercase << num << append;
    out.flags(f);
    out.fill(' ');
}

inline void flush_hex(uint8_t* buffer, const uint16_t& size) {
    cout.fill('0');
    for (int row = 0; row < size / 16; row++) {
        for (int col = 0; col < 16; col++) {
            cout << setw(2) << hex << (int)buffer[row * 16 + col] << " ";
        }
        cout << endl;
    }
    cout << dec << endl;
    cout.fill(' ');
}

#endif //UTIL_H
