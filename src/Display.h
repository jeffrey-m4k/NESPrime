#pragma once

#include <deque>
#include <vector>
#include <array>
#include <SDL.h>
#include "BitUtils.h"
#include "Component.h"

#define WIDTH 256
#define HEIGHT 240

class ExpansionChip;

class Display : public Component
{
public:
	Display()
	{
		init();
	};

	~Display();

	bool init();
	
	void reset();

	bool refresh();

	bool update_pt();

	bool update_nt();

	bool update_apu();

	void close();

	void black();

	void set_pixel_buffer( u8 x, u8 y, const u8 rgb[3] );

	void write_pt_pixel( u8 tile, u8 x, u8 y, bool pt2, const u8 rgb[3] );

	void write_nt_pixel( int tile, u8 x, u8 y, short nt, const u8 rgb[3] );

	int get_apu_channel_from_y( int y );

	int get_apu_chip_from_y( int y );

	void on_left_clicked( int y );

	void on_right_clicked( int y );

	void push_buffer();

	u8 *get_pixels()
	{
		return pixels;
	}

	void set_show_sys_texture( bool show );

	void set_show_window( SDL_Window *window, bool show )
	{
		if ( show )
		{
			SDL_ShowWindow( window );
		}
		else
		{
			SDL_HideWindow( window );
		}
	}

	SDL_Window *get_main_window()
	{
		return window_main;
	}

	SDL_Window* get_nt_window()
	{
		return window_nt;
	}

	SDL_Window* get_pt_window()
	{
		return window_pt;
	}

	SDL_Window* get_apu_window()
	{
		return window_apu;
	}

	SDL_Renderer *get_main_renderer()
	{
		return renderer_main;
	}

	SDL_Texture *get_base_texture()
	{
		return texture_main_base;
	}

	SDL_Texture *get_sys_texture()
	{
		return texture_main_ui;
	}

	void set_sys_texture( SDL_Texture *tex )
	{
		texture_main_ui = tex;
	}

	const void push_apu_samples( std::vector< float > &samples );

	void init_apu_display();

	void apu_debug_solo( int channel );

	std::vector<ExpansionChip *> get_ecs()
	{
		return apu_ecs;
	}

public:
	u32 last_update = 0;
	bool *apu_debug_muted = new bool[5] { false };

private:
	SDL_Window *window_main;
	SDL_Window *window_pt;
	SDL_Window *window_nt;
	SDL_Window *window_apu;
	SDL_Renderer *renderer_main;
	SDL_Renderer *renderer_pt;
	SDL_Renderer *renderer_nt;
	SDL_Renderer *renderer_apu;
	SDL_Texture *texture_game;
	SDL_Texture *texture_pt;
	SDL_Texture *texture_nt;
	SDL_Texture *texture_apu;

	static const int APU_CHANNEL_HEIGHT = 80;
	static const int APU_CHANNEL_PADDING = APU_CHANNEL_HEIGHT / 4;
	static const int APU_CHANNEL_WAVEFORM_HEIGHT = APU_CHANNEL_HEIGHT - APU_CHANNEL_PADDING * 2;
	static const int APU_CHANNEL_HEADER_HEIGHT = 20;

	int apu_channels = 5;
	std::vector<std::array<u8, 3>> apu_chip_colors;
	std::vector<std::array<u8, 3>> apu_channel_colors;
	std::vector<std::string> apu_chip_names;
	std::vector<std::string> apu_channel_names;
	std::vector<ExpansionChip *> apu_ecs;

	static const int APU_BUFFER_SIZE = 4000;
	static const int APU_TRIGGER_WINDOW = APU_BUFFER_SIZE - 680;
	static const int APU_TRIGGER_WINDOW_START = (APU_BUFFER_SIZE - APU_TRIGGER_WINDOW) / 2;
	std::deque< float > waveform_buffers[ 20 ];

	int apu_last_solo = -1;

	void create_apu_base_texture();

	void enqueue_sample( int channel, float sample );

	int get_waveform_trigger( int channel );

	int get_chip_number( int channel );
	
	int get_channel_number( int channel );

	int get_apu_window_height()
	{
		return APU_CHANNEL_HEIGHT *apu_channels + APU_CHANNEL_HEADER_HEIGHT * (apu_ecs.size() + 1);
	}

	int get_channel_top_y( int channel )
	{
		return channel * APU_CHANNEL_HEIGHT + (get_chip_number( channel ) + 1) * APU_CHANNEL_HEADER_HEIGHT;
	}

	void draw_apu_text( const std::string &text, int x, int y, float scale, float opacity );

	u8 pixels[WIDTH * HEIGHT * 3] = { 0 };
	u8 buffer[WIDTH * HEIGHT * 3] = { 0 };

	u8 pt[256 * 128 * 3] = { 0 };
	u8 nts[256 * 240 * 4 * 3] = { 0 };

	int apu_texture_size;
	u8 *apu_pixels = new u8[ 1 ];
	u8 *apu_base_pixels = new u8[ 1 ];

	u32 fps_lasttime;
	u32 fps_current;
	u32 fps_frames;

	SDL_Texture *texture_main_base;
	SDL_Texture *texture_main_ui = nullptr;
	SDL_Texture *texture_apu_base;
	SDL_Texture *texture_apu_overlay;
	bool show_sys_texture = false;
};