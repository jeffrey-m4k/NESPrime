#include "src/nes.h"

#include <iostream>

int main() {
    nes sys;
    loader* l = &sys.ldr;

    if (l->open_file("smb.nes")) {
        std::ifstream& file = l->get_file();
        l->load();
    }
}