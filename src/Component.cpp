#include "Component.h"

NES* Component::get_nes() {
    return this->nes;
}

void Component::set_nes(NES *nes) {
    this->nes = nes;
}