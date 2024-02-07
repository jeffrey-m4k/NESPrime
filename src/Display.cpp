#include "Display.h"
#include "APU/APU.h"
#include "PPU.h"
#include "UI.h"
#include <SDL.h>
#include <SDL_ttf.h>

bool Display::init()
{
	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(0, &mode);

	window_pt = SDL_CreateWindow( "Pattern Tables", 0, 28, 512, 256, SDL_WINDOW_RESIZABLE );
	window_nt = SDL_CreateWindow( "Nametables", 0, 312, 512, 480, SDL_WINDOW_RESIZABLE );
	window_apu = SDL_CreateWindow( "APU Channels", mode.w - APU_WINDOW_WIDTH, 28, APU_WINDOW_WIDTH, 640, SDL_WINDOW_RESIZABLE );
	window_main = SDL_CreateWindow( "NESPrime",
	                                512,
	                                SDL_WINDOWPOS_CENTERED,
	                                WIDTH * 3,
	                                HEIGHT * 3,
	                                SDL_WINDOW_RESIZABLE );

	if ( window_main == nullptr || window_pt == nullptr || window_nt == nullptr || window_apu == nullptr )
	{
		SDL_Log( "Unable to create window: %s", SDL_GetError() );
		return false;
	}
//    SDL_HideWindow(window_pt);
//    SDL_HideWindow(window_nt);

	renderer_main = SDL_CreateRenderer( window_main, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE );
	renderer_pt = SDL_CreateRenderer( window_pt, -1, SDL_RENDERER_ACCELERATED );
	renderer_nt = SDL_CreateRenderer( window_nt, -1, SDL_RENDERER_ACCELERATED );
	renderer_apu = SDL_CreateRenderer( window_apu, -1, SDL_RENDERER_ACCELERATED );

	if ( renderer_main == nullptr || renderer_pt == nullptr || renderer_nt == nullptr || renderer_apu == nullptr )
	{
		SDL_Log( "Unable to create renderer: %s", SDL_GetError() );
		return false;
	}

	SDL_RenderSetLogicalSize( renderer_main, WIDTH * 4, HEIGHT * 4 );
	SDL_RenderSetLogicalSize( renderer_pt, 256, 128 );
	SDL_RenderSetLogicalSize( renderer_nt, 512, 480 );
	SDL_RenderSetLogicalSize( renderer_apu, APU_WINDOW_WIDTH, 640 );

	texture_game = SDL_CreateTexture( renderer_main, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH,
	                                  HEIGHT );
	texture_pt = SDL_CreateTexture( renderer_pt, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 128 );
	texture_nt = SDL_CreateTexture( renderer_nt, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 512, 480 );
	texture_apu = SDL_CreateTexture( renderer_apu, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, APU_WINDOW_WIDTH, 640 );
	if ( texture_game == nullptr || texture_pt == nullptr || texture_nt == nullptr || texture_apu == nullptr )
	{
		SDL_Log( "Unable to create texture: %s", SDL_GetError() );
		return false;
	}


	texture_main_base = SDL_CreateTexture( renderer_main, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, WIDTH * 4,
	                                       HEIGHT * 4 );
	texture_main_ui = SDL_CreateTexture( renderer_main, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, WIDTH * 4,
	                                     HEIGHT * 4 );
	SDL_SetTextureBlendMode( texture_game, SDL_BLENDMODE_BLEND );
	SDL_SetTextureBlendMode( texture_main_ui, SDL_BLENDMODE_BLEND );
	SDL_SetRenderDrawBlendMode( renderer_main, SDL_BLENDMODE_BLEND );

	fps_lasttime = SDL_GetTicks();
	fps_current = 0;
	fps_frames = 0;

	return true;
}

void Display::reset()
{
	for ( int i = 0; i < APU_CHANNELS; ++i )
	{
		nes->get_apu()->set_debug_mute( apu_debug_muted[ i ], i );
	}
}

bool Display::update_pt()
{
	int texture_pitch = 0;
	void *texture_pixels = nullptr;
	if ( SDL_LockTexture( texture_pt, nullptr, &texture_pixels, &texture_pitch ) == 0 )
	{
		memcpy( texture_pixels, pt, texture_pitch * 128 );
	}
	SDL_UnlockTexture( texture_pt );

	SDL_RenderClear( renderer_pt );
	SDL_RenderCopy( renderer_pt, texture_pt, nullptr, nullptr );
	SDL_RenderPresent( renderer_pt );

	return true;
}

bool Display::update_nt()
{
	int texture_pitch = 0;
	void *texture_pixels = nullptr;
	if ( SDL_LockTexture( texture_nt, nullptr, &texture_pixels, &texture_pitch ) == 0 )
	{
		memcpy( texture_pixels, nts, texture_pitch * 480 );
	}
	SDL_UnlockTexture( texture_nt );

	SDL_RenderClear( renderer_nt );
	SDL_RenderCopy( renderer_nt, texture_nt, nullptr, nullptr );
	SDL_RenderPresent( renderer_nt );

	return true;
}

bool Display::update_apu()
{
	uint8_t buf[ APU_WINDOW_WIDTH * 640 * 3 ] = { 0 };

	nes->get_apu()->clear_dmc_waveform();
	for ( int c = 0; c < APU_CHANNELS; ++c )
	{
		float last_sample = nes->get_apu()->get_waveform_at_time( (-321 / 640.0) * APU_WAVEFORM_LENGTH_SECONDS, c );
		for ( int s = 0; s < 640; ++s )
		{
			float time = (s - 320) / 640.0 * APU_WAVEFORM_LENGTH_SECONDS;
			float sample = nes->get_apu()->get_waveform_at_time( time, c );

			uint8_t rgb[ 3 ];
			memcpy( rgb, APU_CHANNEL_COLORS[ c ], 3 );
			if ( apu_debug_muted[ c ] )
			{
				for ( int i = 0; i < 3; ++i )
				{
					rgb[ i ] *= 0.5;
				}
			}

			int diff = sample * APU_CHANNEL_WAVEFORM_WIDTH 
				- last_sample * APU_CHANNEL_WAVEFORM_WIDTH;

			int start_x = c * APU_CHANNEL_WIDTH + APU_CHANNEL_PADDING 
				+ (diff > 0 ? last_sample : sample) * APU_CHANNEL_WAVEFORM_WIDTH;

			for ( int d = 0; d <= abs(diff); ++d )
			{
				if ( start_x + d < APU_WINDOW_WIDTH )
				{
					int buf_idx_a = (s * APU_WINDOW_WIDTH + start_x + d) * 3;
					memcpy( &buf[ buf_idx_a ], rgb, 3 );
				}
			}

			last_sample = sample;
		}
	}

	int texture_pitch = 0;
	void* texture_pixels = nullptr;
	if ( SDL_LockTexture( texture_apu, nullptr, &texture_pixels, &texture_pitch ) == 0 )
	{
		memcpy( texture_pixels, buf, texture_pitch * 640 );
	}
	SDL_UnlockTexture( texture_apu );

	SDL_RenderClear( renderer_apu );
	SDL_RenderCopy( renderer_apu, texture_apu, nullptr, nullptr );
	SDL_RenderPresent( renderer_apu );

	return true;
}

bool Display::refresh()
{
	int texture_pitch = 0;
	void *texture_pixels = nullptr;
	if ( SDL_LockTexture( texture_game, nullptr, &texture_pixels, &texture_pitch ) != 0 )
	{
		SDL_Log( "Unable to lock texture: %s", SDL_GetError() );
	}
	else
	{
		memcpy( texture_pixels, pixels, texture_pitch * HEIGHT );
	}
	SDL_UnlockTexture( texture_game );

	SDL_RenderClear( renderer_main );
	SDL_SetRenderTarget( renderer_main, texture_main_base );
	SDL_RenderCopy( renderer_main, texture_game, nullptr, nullptr );

	if ( nes->get_ui()->get_show() )
	{
		nes->get_ui()->draw();
	}

	SDL_SetRenderTarget( renderer_main, nullptr );
	SDL_RenderCopy( renderer_main, texture_main_base, nullptr, nullptr );
	SDL_RenderPresent( renderer_main );

	fps_frames++;
	Uint32 t = SDL_GetTicks();
	if ( fps_lasttime < t - 1000 )
	{
		fps_lasttime = t;
		fps_current = fps_frames;
		fps_frames = 0;
	}
	SDL_SetWindowTitle( window_main, ("[" + nes->filename + "] FPS:" + std::to_string( fps_current )).c_str() );

	//last_update = SDL_GetTicks();

	return true;
}

void Display::set_show_sys_texture( bool show )
{
	show_sys_texture = show;
}

void Display::black()
{
	for ( int p = 0; p < WIDTH * HEIGHT * 3; p += 3 )
	{
		pixels[p] = 0;
		pixels[p + 1] = 0;
		pixels[p + 2] = 0;
	}
}

void Display::close()
{
	SDL_DestroyTexture( texture_game );
	SDL_DestroyTexture( texture_pt );
	SDL_DestroyTexture( texture_nt );
	SDL_DestroyTexture( texture_main_ui );
	SDL_DestroyRenderer( renderer_main );
	SDL_DestroyRenderer( renderer_pt );
	SDL_DestroyRenderer( renderer_nt );
	SDL_DestroyWindow( window_main );
	SDL_DestroyWindow( window_pt );
	SDL_DestroyWindow( window_nt );
	SDL_Quit();
}

void Display::set_pixel_buffer( uint8_t x, uint8_t y, const uint8_t rgb[3] )
{
	buffer[(x + (y * WIDTH)) * 3] = rgb[0];
	buffer[(x + (y * WIDTH)) * 3 + 1] = rgb[1];
	buffer[(x + (y * WIDTH)) * 3 + 2] = rgb[2];
}

void Display::push_buffer()
{
	//black();
	for ( int p = 0; p < WIDTH * HEIGHT * 3; p++ )
	{
		pixels[p] = buffer[p];
		buffer[p] = 0;
	}
}

void Display::write_pt_pixel( uint8_t tile, uint8_t x, uint8_t y, bool pt2, const uint8_t rgb[3] )
{
	int index = (128 * pt2 + 256 * 8 * (tile / 16) + 8 * (tile % 16) + x + 256 * y) * 3;
	pt[index] = rgb[0];
	pt[index + 1] = rgb[1];
	pt[index + 2] = rgb[2];
}

void Display::write_nt_pixel( int tile, uint8_t x, uint8_t y, short nt, const uint8_t *rgb )
{
	int index = (256 * (nt % 2) + (256 * 240 * 2) * (nt / 2) + 512 * 8 * (tile / 32) + 8 * (tile % 32) + x + 512 * y) * 3;
	nts[index] = rgb[0];
	nts[index + 1] = rgb[1];
	nts[index + 2] = rgb[2];
}