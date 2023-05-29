#include "NES.h"

NES::NES(CPU* cpu) : cpu(cpu) {
    cart = new Cartridge(this);
}

void NES::run() {
    cart->load();
    cpu->run();
}
