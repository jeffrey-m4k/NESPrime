#ifndef NES_H
#define NES_H


#include "processor.h"
#include "loader.h"

class nes {
public:
    nes();
    ~nes() {};
    void run();
public:
    loader ldr = loader(mem);
private:
    memory mem;
    processor cpu = processor(mem);
};


#endif //NES_H
