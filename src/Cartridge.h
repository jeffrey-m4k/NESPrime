#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <fstream>
#include <ios>
#include <string>
#include <vector>
#include <cstdint>
#include "Memory.h"
#include "NES.h"
#include "Processor.h"
#include "CPU.h"
#include "PPU.h"

class NES;

class Cartridge {
public:
    Cartridge(NES* system) : sys(system) {};
    ~Cartridge() {};
    void load();
    bool open_file(const std::string& filename);
    std::ifstream& get_file() { return file; }
private:
    bool read_next(uint16_t bytes = 1);
    bool read_next(uint8_t* into, uint16_t bytes = 1);
    bool read_next(Memory& into, uint16_t start, uint16_t bytes = 1);
    bool read_header();
    void print_metadata();
private:
    static const int BUFFER_SIZE = 16;
    uint8_t buffer[BUFFER_SIZE];
    std::ifstream file;
    uint16_t pos = 0;
    uint16_t prg_size, chr_size;
    uint8_t mapper;
    bool flags[2][4];

    Memory prg_rom;
    Memory chr_rom;

    NES* sys;
};


#endif //CARTRIDGE_H
