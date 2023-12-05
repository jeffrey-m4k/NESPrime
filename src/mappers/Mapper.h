#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include "../Cartridge.h"
#include "../PPU.h"
#include "../CPU.h"

enum MIRRORING {
    Horizontal, Vertical, OneScreen, FourScreen
};

class Mapper {
public:
    explicit Mapper(Cartridge* cart);
    ~Mapper() = default;
    virtual uint8_t* map_cpu(uint16_t addr);
    virtual uint8_t* map_ppu(uint16_t addr);
    void set_mirroring(MIRRORING mirr) { mirroring = mirr; }
    MIRRORING get_mirroring() { return mirroring; }
    void set_bank(uint8_t num) {
        bank = num & 0xF;
    }
protected:
    Cartridge* cartridge;
    MIRRORING mirroring;
    uint8_t bank = 0;

    uint8_t* cpu_mem;
    uint8_t* ppu_mem;
    uint8_t* prg_rom;
    uint8_t* chr_rom;
    uint8_t* prg_ram;
    uint8_t* chr_ram;
};


class Mapper0 : public Mapper {
public:
    explicit Mapper0(Cartridge* cart) : Mapper(cart) { nrom_256 = cart->get_prg_size() == 0x8000; }
    uint8_t* map_cpu(uint16_t address) override {
        if (address < 0x8000) return Mapper::map_cpu(address);
        if (!nrom_256) address &= ~0x4000;
        address -= 0x8000;
        return prg_rom + address;
    };
private:
    bool nrom_256;
};


class Mapper2 : public Mapper {
public:
    explicit Mapper2(Cartridge* cart) : Mapper(cart) { prg_size = cart->get_prg_size(); }
    uint8_t* map_cpu(uint16_t address) override {
        if (address < 0x8000) return Mapper::map_cpu(address);
        if (address >= 0xC000) return prg_rom + (prg_size - 0x4000) + (address - 0xC000);
        else return prg_rom + (bank * 0x4000) + (address - 0x8000);
    }
private:
    uint32_t prg_size;
};

#endif //MAPPER_H
