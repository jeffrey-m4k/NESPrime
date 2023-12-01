#ifndef COMPONENT_H
#define COMPONENT_H

#include "NES.h"

class Component {
public:
    Component() = default;
    ~Component() = default;
    NES* get_nes();
    void set_nes(NES* nes);
protected:
    NES* nes{};
};


#endif
