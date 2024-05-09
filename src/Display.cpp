#include <format>
#include <algorithm>
#include "Display.h"
#include "APU/APU.h"
#include "APU/SoundChip.h"
#include "PPU.h"
#include "UI.h"
#include <SDL.h>
#include <SDL_ttf.h>

bool Display::init()
{
	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(0, &mode);

	window_pt = SDL_CreateWindow( "Pattern Tables", 0, 28, 512, 256, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN );
	window_nt = SDL_CreateWindow( "Nametables", 0, 312, 512, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN );
	window_apu = SDL_CreateWindow( "APU Channels", mode.w - apu_window_width, 28, apu_window_width, get_apu_window_height_min(), SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
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

	renderer_main = SDL_CreateRenderer( window_main, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE );
	renderer_pt = SDL_CreateRenderer( window_pt, -1, SDL_RENDERER_ACCELERATED );
	renderer_nt = SDL_CreateRenderer( window_nt, -1, SDL_RENDERER_ACCELERATED );
	renderer_apu = SDL_CreateRenderer( window_apu, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE );

	if ( renderer_main == nullptr || renderer_pt == nullptr || renderer_nt == nullptr || renderer_apu == nullptr )
	{
		SDL_Log( "Unable to create renderer: %s", SDL_GetError() );
		return false;
	}

	SDL_RenderSetLogicalSize( renderer_main, WIDTH * 4, HEIGHT * 4 );
	SDL_RenderSetLogicalSize( renderer_pt, 256, 128 );
	SDL_RenderSetLogicalSize( renderer_nt, 512, 480 );
	SDL_RenderSetLogicalSize( renderer_apu, apu_window_width, get_apu_window_height_min() );

	texture_game = SDL_CreateTexture( renderer_main, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT );
	texture_pt = SDL_CreateTexture( renderer_pt, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 128 );
	texture_nt = SDL_CreateTexture( renderer_nt, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 512, 480 );
	texture_apu = SDL_CreateTexture( renderer_apu, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, apu_window_width, get_apu_window_height_min() );
	if ( texture_game == nullptr || texture_pt == nullptr || texture_nt == nullptr || texture_apu == nullptr )
	{
		SDL_Log( "Unable to create texture: %s", SDL_GetError() );
		return false;
	}

	texture_main_base = SDL_CreateTexture( renderer_main, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, WIDTH * 4, HEIGHT * 4 );
	texture_main_ui = SDL_CreateTexture( renderer_main, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, WIDTH * 4, HEIGHT * 4 );
	SDL_SetTextureBlendMode( texture_game, SDL_BLENDMODE_BLEND );
	SDL_SetTextureBlendMode( texture_main_ui, SDL_BLENDMODE_BLEND );
	SDL_SetRenderDrawBlendMode( renderer_main, SDL_BLENDMODE_BLEND );

	texture_apu_base = SDL_CreateTexture( renderer_apu, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, apu_window_width, get_apu_window_height_min() );
	texture_apu_overlay = SDL_CreateTexture( renderer_apu, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, apu_window_width, get_apu_window_height_min() );
	SDL_SetTextureBlendMode( texture_apu, SDL_BLENDMODE_BLEND );
	SDL_SetTextureBlendMode( texture_apu_overlay, SDL_BLENDMODE_BLEND );
	SDL_SetRenderDrawBlendMode( renderer_apu, SDL_BLENDMODE_BLEND );

	fps_lasttime = SDL_GetTicks();
	fps_current = 0;
	fps_frames = 0;

	return true;
}

void Display::reset()
{
	for ( int i = 0; i < apu_channels; ++i )
	{
		apu_debug_mute_set( i, apu_debug_muted[ i ] );

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

void Display::init_apu_display()
{
	std::vector<std::string> old_chip_names = apu_chip_names;

	apu_channels = 0;
	apu_chip_colors = {};
	apu_channel_colors = {};
	apu_chip_names = {};
	apu_channel_names = {};
	apu_chips = {};

	apu_chips.push_back( this->get_nes()->get_apu()->get_chip( SCType::RICOH_2A03 ) );

	// TODO account for .nsf which may have multiple ECs at once
	SoundChip *sc = this->get_nes()->get_cart()->get_mapper()->get_sound_chip();
	if ( sc != nullptr )
	{
		apu_chips.push_back( sc );
	}

	for ( SoundChip *sc : apu_chips )
	{
		apu_channels += sc->get_channel_count();
		apu_chip_names.push_back( sc->get_name() );
		apu_chip_colors.push_back( sc->get_debug_base_color( 0 ) );

		for ( int c = 0; c < sc->get_channel_count(); ++c )
		{
			apu_channel_names.push_back( sc->get_channel_name( c ) );
			apu_channel_colors.push_back( sc->get_debug_waveform_color( c ) );
		}
	}

	if ( old_chip_names != apu_chip_names )
	{
		apu_window_width = std::max( apu_window_width, 320 );
		apu_window_height = std::max( apu_window_height, get_apu_window_height_min() );
		SDL_SetWindowSize( window_apu, apu_window_width, apu_window_height );
		reinit_apu_window();

		delete[] apu_debug_muted;
		apu_debug_muted = new bool[apu_channels] { false };
		for ( int i = 0; i < apu_channels; ++i )
		{
			apu_debug_muted[ i ] = apu_chips[ get_chip_number( i ) ]->get_channel( get_channel_number( i ) )->debug_muted;
		}
	}
}

void Display::reinit_apu_window()
{
	SDL_SetWindowMinimumSize( window_apu, 320, get_apu_window_height_min() );
	SDL_RenderSetLogicalSize( renderer_apu, apu_window_width, apu_window_height );

	SDL_DestroyTexture( texture_apu );
	SDL_DestroyTexture( texture_apu_base );
	SDL_DestroyTexture( texture_apu_overlay );
	texture_apu = SDL_CreateTexture( renderer_apu, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, apu_window_width, apu_window_height );
	texture_apu_base = SDL_CreateTexture( renderer_apu, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, apu_window_width, apu_window_height );
	texture_apu_overlay = SDL_CreateTexture( renderer_apu, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, apu_window_width, apu_window_height );
	SDL_SetTextureBlendMode( texture_apu, SDL_BLENDMODE_BLEND );
	SDL_SetTextureBlendMode( texture_apu_overlay, SDL_BLENDMODE_BLEND );
	SDL_SetRenderDrawBlendMode( renderer_apu, SDL_BLENDMODE_BLEND );

	apu_texture_size = std::ceil(apu_window_width * 3 / 4.0) * 4 * apu_window_height;
	delete[] apu_pixels;
	delete[] apu_base_pixels;
	apu_pixels = new u8[ apu_texture_size ]{ 0 };
	apu_base_pixels = new u8[ apu_texture_size ]{ 0 };

	create_apu_base_texture();
}

int Display::get_chip_number( int channel )
{
	int chip = 0;
	do
	{
		SoundChip *sc = apu_chips[ chip++ ];
		channel -= sc->get_channel_count();
	}
	while ( channel >= 0 );

	return chip - 1;
}

int Display::get_channel_number( int channel )
{
	return channel - get_first_channel( get_chip_number( channel ) );
}

void Display::draw_apu_text( const std::string &text, int x, int y, float scale, float opacity )
{
	int text_w, text_h;

	TTF_Font *font_ui = nes->get_ui()->get_font();
	SDL_Surface *text_surf = TTF_RenderText_Solid( nes->get_ui()->get_font(), text.c_str(), { 255, 255, 255 } );
	SDL_SetSurfaceAlphaMod( text_surf, opacity * 255.0 );
	SDL_Texture *text_texture = SDL_CreateTextureFromSurface( renderer_apu, text_surf );
	TTF_SizeText( font_ui, text.c_str(), &text_w, &text_h );
	SDL_Rect text_rect{ x, y - 4 * scale, text_w * scale, text_h * scale };
	SDL_RenderCopy( renderer_apu, text_texture, nullptr, &text_rect );
	SDL_FreeSurface( text_surf );
	SDL_DestroyTexture( text_texture );
}

void Display::create_apu_base_texture()
{
	std::fill( apu_base_pixels, apu_base_pixels + apu_texture_size, 40 );

	bool odd = true;

	SDL_SetRenderTarget( renderer_apu, texture_apu_overlay );
	SDL_RenderClear( renderer_apu );

	for ( int c = 0; c < apu_channels; ++c )
	{
		draw_apu_text( apu_channel_names[ c ], 0, get_channel_top_y( c ), 0.5, 0.6 );
		if ( get_channel_number( c ) == 0 )
		{
			odd = true;
			draw_apu_text( apu_chip_names[ get_chip_number( c ) ], 0, get_channel_top_y( c ) - APU_CHANNEL_HEADER_HEIGHT, 1.0, 1.0 );
		}

		int top_left_idx = get_channel_top_y( c ) * apu_window_width * 3;

		int z = 0;
		int h = 0;
		std::generate( apu_base_pixels + top_left_idx, apu_base_pixels + top_left_idx + (apu_window_width * get_apu_channel_height() * 3), [&] {
			double out;
			if ( h / (apu_window_width * 3) == (get_apu_channel_height() / 2) )
				out = 50.0;
			else if ( h / 3 % apu_window_width == (apu_window_width / 2) ) 
				out = 20.0;
			else 
				out = apu_chip_colors[ get_chip_number( c ) ][ (z % 3) ] * (odd ? 0.06 : 0.03) * (1 - 0.3 * (h / (apu_window_width * 3) / (float)get_apu_channel_height()));
			h++;
			z++;
			return out;
		} );
		odd = !odd;
	}

	SDL_SetRenderTarget( renderer_apu, nullptr );
}

bool Display::update_apu()
{
	std::copy( apu_base_pixels, apu_base_pixels + apu_texture_size, apu_pixels );

	for ( int c = 0; c < apu_channels; ++c )
	{
		int trigger = get_waveform_trigger( c );
		if ( trigger == -1 )
		{
			continue;
		}

		float last_sample = -waveform_buffers[ c ][ trigger - (apu_window_width / 2 - 1) ] / 2.0 + 0.5;
		for ( int s = 0; s < apu_window_width; ++s )
		{
			float sample = -waveform_buffers[ c ][ trigger + s - (apu_window_width / 2) ] / 2.0 + 0.5;

			std::array<u8, 3> rgb = apu_channel_colors[ c ];
			if ( apu_debug_muted[ c ] )
			{
				for ( int i = 0; i < 3; ++i )
				{
					rgb[ i ] *= 0.25;
				}
			}

			int diff = sample * get_apu_channel_waveform_height()
				- last_sample * get_apu_channel_waveform_height();

			int start_y = get_channel_top_y( c ) + get_apu_channel_padding()
				+ (diff > 0 ? last_sample : sample) * get_apu_channel_waveform_height();

			for ( int d = 0; d <= abs(diff); ++d )
			{
				if ( (start_y + d < (apu_window_height)) && (start_y + d >= 0) )
				{
					int buf_idx_a = (s + apu_window_width * (start_y + d)) * 3;
					std::copy( rgb.begin(), rgb.end(), apu_pixels + buf_idx_a );
				}
			}

			last_sample = sample;
		}
	}

	int texture_pitch = 0;
	void* texture_pixels = nullptr;
	if ( SDL_LockTexture( texture_apu, nullptr, &texture_pixels, &texture_pitch ) == 0 )
	{
		memcpy( texture_pixels, apu_pixels, texture_pitch * apu_window_height );
	}
	SDL_UnlockTexture( texture_apu );

	SDL_RenderClear( renderer_apu );

	SDL_SetRenderTarget( renderer_apu, texture_apu_base );
	SDL_RenderCopy( renderer_apu, texture_apu, nullptr, nullptr );
	SDL_RenderCopy( renderer_apu, texture_apu_overlay, nullptr, nullptr );

	SDL_SetRenderTarget( renderer_apu, nullptr );
	SDL_RenderCopy( renderer_apu, texture_apu_base, nullptr, nullptr );
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
	for ( int i = 0; i < samples.size() && i < 20; ++i )
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
	int greatest_crossing = get_apu_trigger_window_start();

	float last = waveform_buffers[ channel ][ get_apu_trigger_window_start()];

	for ( int i = get_apu_trigger_window_start() + 1; i < get_apu_trigger_window_start() + get_apu_trigger_window(); ++i )
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

void Display::apu_debug_mute_set( int channel, bool mute )
{
	apu_chips[ get_chip_number( channel ) ]->get_channel( get_channel_number( channel ) )->debug_muted = mute;
	apu_debug_muted[ channel ] = mute;
}

void Display::apu_debug_solo( int channel )
{
	if ( get_chip_number( channel ) == -1 ) return;
	for ( int c = 0; c < apu_channels; ++c )
	{
		apu_debug_mute_set( c, apu_last_solo == channel ? false : (c != channel) );
	}
	apu_last_solo = apu_last_solo == channel ? -1 : channel;
}

int Display::get_apu_channel_from_y( int y )
{
	for ( int i = 0; i < apu_channels; ++i )
	{
		int dist = y - get_channel_top_y( i );
		if ( dist < get_apu_channel_height() && dist > 0 )
			return i;
	}
	return -1;
}

int Display::get_apu_chip_from_y( int y )
{
	int c = 0;
	for ( int i = 0; i < apu_chips.size(); ++i )
	{
		int start_y = i * APU_CHANNEL_HEADER_HEIGHT + c * get_apu_channel_height();
		if ( y >= start_y && y < start_y + APU_CHANNEL_HEADER_HEIGHT )
			return i;
		c += apu_chips[ i ]->get_channel_count();
	}
	return -1;
}

void Display::on_left_clicked( int y )
{
	int c;
	if ( (c = get_apu_channel_from_y( y )) != -1 )
	{
		apu_debug_mute_set( c, !(apu_chips[ get_chip_number( c ) ]->get_channel( get_channel_number( c ) )->debug_muted) );
	}
	else if ( (c = get_apu_chip_from_y( y )) != -1 )
	{
		bool is_any_on = false;
		for ( int i = 0; i < apu_chips[ c ]->get_channel_count(); ++i )
		{
			if ( !apu_chips[ c ]->get_channel( i )->debug_muted )
			{
				is_any_on = true;
				break;
			}
		}
		for ( int i = 0; i < apu_chips[ c ]->get_channel_count(); ++i )
		{
			apu_chips[ c ]->get_channel( i )->set_debug_mute( is_any_on );
			apu_debug_muted[ get_first_channel( c ) + i ] = is_any_on;
		}
	}
}

void Display::on_right_clicked( int y )
{
	int c;
	if ( (c = get_apu_channel_from_y( y )) != -1 )
	{
		apu_debug_solo( c );
	}
}

void Display::on_apu_window_resized()
{
	update_apu_window_size();
	reinit_apu_window();
}

int Display::get_first_channel( int chip )
{
	int c = 0;
	for ( int i = 0; i < chip; ++i )
	{
		c += apu_chips[ i ]->get_channel_count();
	}
	return c;
}