#include "loader.h"

#include <iostream>

int main() {
    loader* l = new loader();

    if (l->open_file("smb.nes")) {
        std::ifstream& file = l->get_file();
        l->load();
    }
}