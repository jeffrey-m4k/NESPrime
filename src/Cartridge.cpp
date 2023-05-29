#include "Cartridge.h"
#include "util.h"

#include <iostream>

using std::ios;

bool Cartridge::read_next(const uint16_t &bytes) {
    return read_next(buffer, bytes);
}

bool Cartridge::read_next(uint8_t* into, const uint16_t &bytes) {
    // TODO better EOF checking, for now just assuming ROM is formatted correctly
    if (!file.eof()) {
        file.seekg(pos);
        file.read((char*)into, bytes);
        pos += bytes;
        return true;
    }
    return false;
}

bool Cartridge::read_next(Memory& into, const uint16_t &start, const uint16_t &bytes) {
    if (!file.eof()) {
        file.seekg(pos);
        if (start + bytes >= into.get_size()) return false;
        file.read((char*)into.get_mem(), bytes);
        pos += bytes;
        return true;
    }
    return false;
}

bool Cartridge::open_file(const std::string &filename) {
    pos = 0;
    file.open(filename, ios::in | ios::binary);
    return file.is_open();
}

bool Cartridge::load_header() {
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
        // TODO support other mappers
        if (mapper != 0) return false;
        for (int i = 0; i < 4; i++) {
            flags[0][i] = (buffer[6] >> i) & 0x1;
            flags[1][i] = (buffer[7] >> i) & 0x1;
        }

        print_hex(buffer, 16);
        return true;
    }
    return false;
}

void Cartridge::load() {
    if (load_header()) {
        print_metadata();

        // If trainer is present, skip past it
        // TODO add trainer support (low priority)
        if (flags[0][2]) {
            pos += 512;
        }

        prg_rom.init(prg_size);
        read_next(prg_rom, prg_size);

        if (chr_size) {
            chr_rom.init(chr_size);
            read_next(chr_rom, chr_size);
        }

        CPU* cpu = sys->get_cpu();
        uint8_t* prg_mem = prg_rom.get_mem();
        switch (mapper) {
            case 0: {
                bool nrom_256 = prg_size == 0x8000;
                cpu->map(0x8000, prg_mem, prg_size);
                if (!nrom_256)
                    cpu->map(0xC000, prg_mem, prg_size);
                break;
            }
            default:
                break;
        }

        // TODO add PlayChoice-10 support (low priority)
    }
    else
        std::cout << "Invalid NES file\n";
}

void Cartridge::print_metadata() {
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


