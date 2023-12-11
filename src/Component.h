#pragma once

#include "NES.h"

class Component
{
public:
	Component() = default;

	~Component() = default;

	NES *get_nes();

	void set_nes( NES *nes );

protected:
	NES *nes{};
};
