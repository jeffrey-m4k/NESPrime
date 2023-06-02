#include "NES.h"

NES::NES(CPU* cpu, PPU* ppu) : cpu(cpu), ppu(ppu) {
    cart = new Cartridge(this);
}

void NES::run() {
    cpu->link_ppu(ppu);
    cart->load();
    cpu->run();
}
