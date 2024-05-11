#pragma once

#include "BitUtils.h"
#include "Component.h"
#include "Display.h"
#include "SDL_ttf.h"
#include "SDL_image.h"
#include <filesystem>
#include <utility>
#include <vector>
#include <nfd.h>

enum class UIState
{
	MAIN, PAUSE
};

enum class Color
{
	WHITE, BLACK, RED, NONE
};

enum class HAlign
{
	LEFT, CENTER, RIGHT
};

enum class VAlign
{
	TOP, CENTER, BOTTOM
};

class Button
{
public:
	Button( std::string text, int x, int y ) : text( std::move( text ) ), x( x ), y( y )
	{
	};

	void set_selected( bool s )
	{
		selected = s;
	}

	void set_show( bool s )
	{
		show = s;
	}

	std::string get_text()
	{
		return text;
	}

	bool get_selected() const
	{
		return selected;
	}

	int get_x() const
	{
		return x;
	}

	int get_y() const
	{
		return y;
	}

private:
	std::string text;
	int x;
	int y;

	bool selected = false;
	bool show = false;
};

class UI : public Component
{
public:
	bool init();

	void tick();

	void draw();

	void handle( SDL_Event &e );

	void handle_global( SDL_Event &e );

	void set_show( bool s )
	{
		show = s;
		needs_update = s;
		nes->get_display()->set_show_sys_texture( s );
	}

	bool get_show() const
	{
		return show;
	}

	void set_state( UIState s )
	{
		state = s;
		needs_update = true;
	}

	UIState get_state() const
	{
		return state;
	}

	TTF_Font *get_font() const
	{
		return font_ui;
	}

private:
	void set_render_draw_color( Color col, u8 alpha );

	void draw_text( const std::string &text, int x, int y, float scale, HAlign h_align, VAlign v_align,
	                Color col = Color::WHITE, Color bgr_col = Color::NONE );

	void show_rom_dialog();

private:
	bool show = false;
	bool needs_update = true;
	UIState state = UIState::MAIN;

	SDL_Texture *texture_ui;
	SDL_Texture *texture_base;
	SDL_Renderer *renderer_ui;
	TTF_Font *font_ui;
	SDL_Surface *splash_img;

	static constexpr SDL_Rect screen_rect{0, 0, WIDTH * 4, HEIGHT * 4};
	static constexpr SDL_Color colors[] = {
			{255, 255, 255},
			{0,   0,   0},
			{255, 0,   0}
	};

	// === STATE: MAIN
	nfdchar_t *outPath = nullptr;
	Uint32 start_tick;
	bool show_splash = true;
	bool cart_error = false;

	// === STATE: PAUSE
	std::vector< Button * > pause_buttons;
};