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
    virtual void set_bank(uint8_t num) { bank = num; }
    void set_mirroring(MIRRORING mirr) { mirroring = mirr; }
    MIRRORING get_mirroring() { return mirroring; }
protected:
    Cartridge* cartridge;
    MIRRORING mirroring;
    uint8_t bank = 0;
    uint32_t prg_size;
    uint32_t chr_size;

    uint8_t* cpu_mem;
    uint8_t* ppu_mem;
    uint8_t* prg_rom;
    uint8_t* chr_rom;
    uint8_t* prg_ram;
    uint8_t* chr_ram;
};


class Mapper2 : public Mapper {
public:
    explicit Mapper2(Cartridge* cart) : Mapper(cart) {};
    uint8_t* map_cpu(uint16_t address) override {
        if (address < 0x8000) return Mapper::map_cpu(address);
        if (address >= 0xC000) return prg_rom + (prg_size - 0x4000) + (address - 0xC000);
        else return prg_rom + (bank * 0x4000) + (address - 0x8000);
    }
    void set_bank(uint8_t num) override {
        bank = num & ((prg_size / 0x4000) - 1);
    }
};

// TODO emulate bus conflicts (e.g. Cybernoid)
class Mapper3 : public Mapper {
public:
    explicit Mapper3(Cartridge* cart) : Mapper(cart) {};
    uint8_t* map_ppu(uint16_t address) override {
        if (address >= 0x2000) return Mapper::map_ppu(address);
        return chr_rom + ((bank+1) * 0x2000) + (address - 0x2000);
    }
    void set_bank(uint8_t num) override {
        bank = num & 0x3;//((chr_size / 0x2000) - 1);
    }
};

#endif //MAPPER_H
