#include "loader.h"
#include "util.h"

#include <iostream>

using std::ios;

bool loader::read_next(const int &bytes) {
    return read_next(buffer, bytes);
}

bool loader::read_next(unsigned char* into, const int &bytes) {
    if (!file.eof()) {
        file.seekg(pos);
        file.read((char*)into, bytes);
        pos += bytes;
        return true;
    }
    return false;
}

bool loader::open_file(const std::string &filename) {
    pos = 0;
    file.open(filename, ios::in | ios::binary);
    return file.is_open();
}

bool loader::load_header() {
    if (read_next(16)) {
        // Check for valid NES header
        if (buffer[0] != 0x4E || buffer[1] != 0x45 || buffer[2] != 0x53 || buffer[3] != 0x1A)
            return false;

        // Check if file is iNES, return false if not
        // TODO add NES2.0 support (low priority)
        if ((buffer[7] & 0xC) != 0x0)
            return false;

        // Read PRG and CHR ROM sizes in bytes
        prg_size = buffer[4] * 0x4000;
        chr_size = buffer[5] * 0x2000;

        mapper = (buffer[6] & 0xF0) | (buffer[7] >> 4);
        for (int i = 0; i < 4; i++) {
            flags[0][i] = (buffer[6] >> i) & 0x1;
            flags[1][i] = (buffer[7] >> i) & 0x1;
        }

        print_hex(buffer, 16);
        return true;
    }
    return false;
}

void loader::load() {
    if (load_header()) {
        print_metadata();

        // If trainer is present, skip past it
        // TODO add trainer support (low priority)
        if (flags[0][2]) {
            pos += 512;
        }

        prg_rom = new unsigned char[prg_size]();
        read_next(prg_rom, prg_size);

        if (chr_size) {
            chr_rom = new unsigned char[chr_size]();
            read_next(chr_rom, chr_size);
        }

        // TODO add PlayChoice-10 support (low priority)
    }
    else
        std::cout << "Invalid NES file\n";
}

void loader::print_metadata() {
    std::cout << "PRG ROM size: " << prg_size << " bytes\n";
    std::cout << "CHR ROM size: " << chr_size << " bytes\n";
    std::cout << "Mapper: " << (int)mapper << "\n";
    std::cout << "Flags:\n";
    std::cout << "0: ";
    for (int i = 0; i < 4; i++)
        std::cout << flags[0][i] << " ";
    std::cout << "\n1: ";
    for (int i = 0; i < 4; i++)
        std::cout << flags[1][i] << " ";
    std::cout << "\n";
}


