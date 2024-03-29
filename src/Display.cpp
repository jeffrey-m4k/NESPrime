#include <format>
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
	window_apu = SDL_CreateWindow( "APU Channels", mode.w - 640, 28, 640, APU_WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE );
	window_main = SDL_CreateWindow( "NESPrime",
	                                SDL_WINDOWPOS_CENTERED,
	                                SDL_WINDOWPOS_CENTERED,
	                                WIDTH * 3,
	                                HEIGHT * 3,
	                                SDL_WINDOW_RESIZABLE );

	if ( window_main == nullptr || window_pt == nullptr || window_nt == nullptr || window_apu == nullptr )
	{
		SDL_Log( "Unable to create window: %s", SDL_GetError() );
		return false;
	}
    SDL_HideWindow(window_pt);
    SDL_HideWindow(window_nt);
	SDL_HideWindow( window_apu );

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
	SDL_RenderSetLogicalSize( renderer_apu, 640, APU_WINDOW_HEIGHT );

	texture_game = SDL_CreateTexture( renderer_main, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH,
	                                  HEIGHT );
	texture_pt = SDL_CreateTexture( renderer_pt, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 128 );
	texture_nt = SDL_CreateTexture( renderer_nt, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 512, 480 );
	texture_apu = SDL_CreateTexture( renderer_apu, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 640, APU_WINDOW_HEIGHT );
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

		for ( int i = 0; i < APU_BUFFER_SIZE; ++i )
		{
			push_apu_samples( std::vector<float> { 0, 0, 0, 0, 0 } );
		}
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
	std::fill(apu_pixels, apu_pixels + 640 * APU_WINDOW_HEIGHT * 3, 0);

	for ( int c = 0; c < APU_CHANNELS; ++c )
	{
		int trigger = get_waveform_trigger( c );
		if ( trigger == -1 )
		{
			continue;
		}

		float last_sample = -waveform_buffers[ c ][ trigger - 321 ] / 2.0 + 0.5;
		for ( int s = 0; s < 640; ++s )
		{
			float sample = -waveform_buffers[ c ][ trigger + s - 320 ] / 2.0 + 0.5;

			u8 midline_rgb[ 3 ]{ 50, 50, 50 };
			int mid_x = (s + c * 640 * APU_CHANNEL_HEIGHT + APU_CHANNEL_PADDING * 640 + 0.5 * 640 * APU_CHANNEL_WAVEFORM_HEIGHT) * 3;
			memcpy( &apu_pixels[ mid_x ], midline_rgb, 3 );

			u8 rgb[ 3 ];
			memcpy( rgb, APU_CHANNEL_COLORS[ c ], 3 );
			if ( apu_debug_muted[ c ] )
			{
				for ( int i = 0; i < 3; ++i )
				{
					rgb[ i ] *= 0.5;
				}
			}

			int diff = sample * APU_CHANNEL_WAVEFORM_HEIGHT
				- last_sample * APU_CHANNEL_WAVEFORM_HEIGHT;

			int start_y = c * APU_CHANNEL_HEIGHT + APU_CHANNEL_PADDING 
				+ (diff > 0 ? last_sample : sample) * APU_CHANNEL_WAVEFORM_HEIGHT;

			for ( int d = 0; d <= abs(diff); ++d )
			{
				if ( (start_y + d < APU_WINDOW_HEIGHT) && (start_y + d >= 0) )
				{
					int buf_idx_a = (s + 640 * (start_y + d)) * 3;
					memcpy( &apu_pixels[ buf_idx_a ], rgb, 3 );
				}
			}

			last_sample = sample;
		}
	}

	int texture_pitch = 0;
	void* texture_pixels = nullptr;
	if ( SDL_LockTexture( texture_apu, nullptr, &texture_pixels, &texture_pitch ) == 0 )
	{
		memcpy( texture_pixels, apu_pixels, texture_pitch * APU_WINDOW_HEIGHT );
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

	//fps_frames++;
	//Uint32 t = SDL_GetTicks();
	//if ( fps_lasttime < t - 1000 )
	//{
	//	fps_lasttime = t;
	//	fps_current = fps_frames;
	//	fps_frames = 0;
	//}
	std::stringstream stream;
	stream << std::fixed << std::setprecision( 2 ) << nes->get_emu_speed();
	SDL_SetWindowTitle( window_main, ("[" + nes->filename + "] Speed: " + stream.str() + "x").c_str());

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

void Display::set_pixel_buffer( u8 x, u8 y, const u8 rgb[3] )
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

void Display::write_pt_pixel( u8 tile, u8 x, u8 y, bool pt2, const u8 rgb[3] )
{
	int index = (128 * pt2 + 256 * 8 * (tile / 16) + 8 * (tile % 16) + x + 256 * y) * 3;
	pt[index] = rgb[0];
	pt[index + 1] = rgb[1];
	pt[index + 2] = rgb[2];
}

void Display::write_nt_pixel( int tile, u8 x, u8 y, short nt, const u8 *rgb )
{
	int index = (256 * (nt % 2) + (256 * 240 * 2) * (nt / 2) + 512 * 8 * (tile / 32) + 8 * (tile % 32) + x + 512 * y) * 3;
	nts[index] = rgb[0];
	nts[index + 1] = rgb[1];
	nts[index + 2] = rgb[2];
}

const void Display::push_apu_samples( std::vector< float > &samples )
{
	samples[ 4 ] /= 8.0;
	for ( int i = 0; i < samples.size() && i < APU_CHANNELS; ++i )
	{
		enqueue_sample( i, samples[ i ] * 2.0 );
	}
}

void Display::enqueue_sample( int channel, float sample )
{
	waveform_buffers[ channel ].push_back( sample );
	if ( waveform_buffers[ channel ].size() > APU_BUFFER_SIZE )
	{
		waveform_buffers[ channel ].pop_front();
	}
}

int Display::get_waveform_trigger( int channel )
{
	if ( waveform_buffers[ channel ].size() < APU_BUFFER_SIZE )
	{
		return -1;
	}

	float rise = 0;
	int grace = 0;
	bool zero_crossed = false;
	int crossed_at = 0;

	float greatest_rise = 0;
	int greatest_crossing = APU_TRIGGER_WINDOW_START;

	float last = waveform_buffers[ channel ][ APU_TRIGGER_WINDOW_START ];

	for ( int i = APU_TRIGGER_WINDOW_START + 1; i < APU_TRIGGER_WINDOW_START + APU_TRIGGER_WINDOW; ++i )
	{
		float sample = waveform_buffers[ channel ][ i ];
		if ( sample >= last )
		{
			rise += sample - last;
			grace = 0;

			if ( sample > 0 && last <= 0 && !zero_crossed )
			{
				zero_crossed = true;
				crossed_at = i;
				if ( channel != 4 )
				{
					return crossed_at;
				}
			}
		}
		else
		{
			if ( ++grace >= 2 )
			{
				if ( zero_crossed && rise > greatest_rise )
				{
					greatest_rise = rise;
					greatest_crossing = crossed_at;
				}
				rise = 0;
				zero_crossed = false;
			}
		}
		last = sample;
	}

	return greatest_crossing;
}

void Display::apu_debug_solo( int channel )
{
	if ( apu_last_solo == channel )
	{
		apu_last_solo = -1;
		for ( int c = 0; c < APU_CHANNELS; ++c )
		{
			nes->get_apu()->set_debug_mute( false, c );
		}
	}
	else
	{
		apu_last_solo = channel;
		for ( int c = 0; c < APU_CHANNELS; ++c )
		{
			nes->get_apu()->set_debug_mute( c != channel, c );
		}
	}
}