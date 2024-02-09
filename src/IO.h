#pragma once

#include "BitUtils.h"
#include "Component.h"
#include <SDL.h>

enum KEY
{
	A = 0, B = 1, SELECT = 2, START = 3, UP = 4, DOWN = 5, LEFT = 6, RIGHT = 7
};

class IO : public Component
{
public:
	void poll();

	bool read_joy();

private:
	constexpr static SDL_Scancode bindings[8] = {
			SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_TAB, SDL_SCANCODE_RETURN,
			SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT
	};
	u8 joy_status[2] = {0};
};
