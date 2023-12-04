#include "IO.h"

void IO::poll() {
    auto* keystate = const_cast<Uint8 *>(SDL_GetKeyboardState(nullptr));
    for (int k = 0; k < 8; k++) {
        joy_status[0] = joy_status[0] & ~(1 << k) | (keystate[bindings[k]] << k);
    }
}

bool IO::read_joy() {
    bool val = (joy_status[0] & 0x1);
    joy_status[0] >>= 1;
    joy_status[0] |= 1 << 7;
    return val;
}