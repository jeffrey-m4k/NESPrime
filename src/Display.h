#pragma once

#include <deque>
#include <vector>
#include <SDL.h>
#include "Component.h"

#define WIDTH 256
#define HEIGHT 240

class Display : public Component
{
public:
	Display()
	{
		init();
	};

	~Display();

	bool init();

	bool refresh();

	bool update_pt();

	bool update_nt();

	bool update_apu();

	void close();

	void black();

	void set_pixel_buffer( uint8_t x, uint8_t y, const uint8_t rgb[3] );

	void write_pt_pixel( uint8_t tile, uint8_t x, uint8_t y, bool pt2, const uint8_t rgb[3] );

	void write_nt_pixel( int tile, uint8_t x, uint8_t y, short nt, const uint8_t rgb[3] );

	void write_apu_sample( float sample, int channel, bool is_playing, bool start_of_sequence )
	{
		if ( apu_samples[ channel ].empty() && !apu_debug_muted[ channel ] && is_playing && !start_of_sequence ) return;
		if (apu_samples[ channel ].size() < APU_SAMPLES_MAX )
		{
			apu_samples[ channel ].push_back( sample );
		}
	}

	void clear_apu_samples()
	{
		for ( int i = 0; i < APU_CHANNELS; ++i )
		{
			apu_samples[ i ].clear();
		}
	}

	int get_apu_channel_from_x( int x )
	{
		return x / APU_CHANNEL_WIDTH;
	}

	void push_buffer();

	uint8_t *get_pixels()
	{
		return pixels;
	}

	void set_show_sys_texture( bool show );

	SDL_Window *get_main_window()
	{
		return window_main;
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

public:
	uint32_t last_update = 0;
	bool apu_debug_muted[4] = {false};

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

	uint8_t pixels[WIDTH * HEIGHT * 3] = {0};
	uint8_t buffer[WIDTH * HEIGHT * 3] = {0};

	uint8_t pt[256 * 128 * 3] = {0};
	uint8_t nts[256 * 240 * 4 * 3] = {0};

	// 0: Pulse 1, 1 : Pulse 2, 2 : Triangle, 3 : Noise
	static const int APU_SAMPLES_MAX = 640;
	static const int APU_WINDOW_WIDTH = 240;
	static const int APU_CHANNELS = 4;
	static const int APU_CHANNEL_WIDTH = APU_WINDOW_WIDTH / APU_CHANNELS;
	static const int APU_CHANNEL_PADDING = APU_CHANNEL_WIDTH / 8;
	static const int APU_CHANNEL_WAVEFORM_WIDTH = APU_CHANNEL_WIDTH - APU_CHANNEL_PADDING * 2;
	std::deque< float > apu_samples[ APU_CHANNELS ];

	uint32_t fps_lasttime;
	uint32_t fps_current;
	uint32_t fps_frames;

	SDL_Texture *texture_main_base;
	SDL_Texture *texture_main_ui = nullptr;
	bool show_sys_texture = false;
};