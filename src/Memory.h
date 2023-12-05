#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>

class Memory {
public:
    Memory() : size(0), mem(nullptr) {};
    Memory(const uint32_t size) : size(size) { mem = new uint8_t[size]; };
    ~Memory() { delete[] mem; };
    void init(uint32_t size);
    uint8_t read(uint32_t addr);
    bool write(uint32_t addr, uint8_t data);
    bool write(uint32_t addr, const uint8_t* data, int bytes);
    uint32_t get_size() const { return size; };
    uint8_t* get_mem() const { return mem; };
private:
    uint32_t size;
    uint8_t* mem;
};


#endif //MEMORY_H
