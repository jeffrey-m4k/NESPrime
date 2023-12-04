#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <fstream>
#include <ios>
#include <string>
#include <vector>
#include <cstdint>
#include "Memory.h"
#include "Component.h"

class Mapper;

class Cartridge : public Component {
public:
    Cartridge() : Component() {};
    ~Cartridge() {};
    void load();
    bool open_file(const std::string& filename);
    std::ifstream& get_file() { return file; }

    uint16_t get_prg_size() const { return prg_size; }
    uint16_t get_chr_size() const { return prg_size; }
    Memory* get_prg() { return &prg_rom; }
    Memory* get_chr() { return &chr_rom; }
    Mapper* get_mapper() { return mapper; }
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
    uint8_t mapper_num;
    bool flags[2][4];

    Memory prg_rom;
    Memory chr_rom;
    Mapper* mapper;
};


#endif //CARTRIDGE_H
