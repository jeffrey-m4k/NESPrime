#include "UI.h"
#include "Cartridge.h"
#include "data.h"

using namespace std::filesystem;

bool UI::init()
{
	texture_ui = nes->get_display()->get_sys_texture();
	texture_base = nes->get_display()->get_base_texture();
	renderer_ui = nes->get_display()->get_main_renderer();

	SDL_RWops *font_mem = SDL_RWFromConstMem( ui_font, sizeof(ui_font) );
	SDL_RWops *splash_mem = SDL_RWFromConstMem( splash, sizeof(splash) );
	font_ui = TTF_OpenFontRW( font_mem, 1, 32 );
	splash_img = IMG_Load_RW( splash_mem, 1 );

	if ( font_ui == nullptr || splash_img == nullptr )
	{
		SDL_Log( "Unable to initialize resource: %s", SDL_GetError() );
		return false;
	}

	show = true;
	state = UIState::MAIN;

	pause_buttons.push_back( new Button( "LOAD NEW ROM", 512, 720 ) );
	pause_buttons.push_back( new Button( "QUIT", 512, 800 ) );
	pause_buttons.at( 0 )->set_selected( true );

	start_tick = SDL_GetTicks();

	return true;
}

void UI::tick()
{
	if ( !show )
	{
		return;
	}
	if ( state == UIState::MAIN && show_splash && SDL_GetTicks() - start_tick > 1250 )
	{
		show_splash = false;
		needs_update = true;
	}
}

void UI::handle( SDL_Event &e )
{
	SDL_Scancode sym = e.key.keysym.scancode;
	switch ( state )
	{
		case UIState::MAIN:
			switch ( sym )
			{
				case SDL_SCANCODE_RETURN:
					if ( show_splash )
					{
						show_splash = false;
						needs_update = true;
					}
					else
					{
						show_rom_dialog();
					}
				default:
					break;
			}
			break;
		case UIState::PAUSE:
			switch ( sym )
			{
				case SDL_SCANCODE_UP:
				case SDL_SCANCODE_DOWN:
					for ( int b = 0; b < pause_buttons.size(); ++b )
					{
						Button *but = pause_buttons.at( b );
						if ( but->get_selected() )
						{
							but->set_selected( false );
							if ( sym == SDL_SCANCODE_DOWN && ++b == pause_buttons.size() )
							{
								pause_buttons.at( 0 )->set_selected( true );
							}
							else if ( sym == SDL_SCANCODE_UP && --b < 0 )
							{
								pause_buttons.at( pause_buttons.size() - 1 )->set_selected( true );
							}
							else
							{
								pause_buttons.at( b )->set_selected( true );
							}
							break;
						}
					}
					needs_update = true;
					break;
				case SDL_SCANCODE_RETURN:
					if ( pause_buttons.at( 0 )->get_selected() )
					{
						show_rom_dialog();
					}
					else if ( pause_buttons.at( 1 )->get_selected() )
					{
						nes->kill();
					}
				default:
					break;
			}
			break;
	}
	handle_global( e );
}

void UI::handle_global( SDL_Event &e )
{
	if ( e.key.keysym.scancode == SDL_SCANCODE_F11 )
	{
		Uint32 window_id;
		if ( (window_id = e.key.windowID) != 0 )
		{
			SDL_Window *win = SDL_GetWindowFromID( window_id );
			if ( SDL_GetWindowFlags( win ) & SDL_WINDOW_FULLSCREEN_DESKTOP )
			{
				SDL_SetWindowFullscreen( win, 0 );
				//SDL_ShowCursor( SDL_ENABLE );
			}
			else
			{
				SDL_SetWindowFullscreen( win, SDL_WINDOW_FULLSCREEN_DESKTOP );
				//SDL_ShowCursor( SDL_DISABLE );
			}
		}
	}
	else if ( SDL_GetModState() & KMOD_CTRL )
	{
		switch ( e.key.keysym.scancode )
		{
			case SDL_SCANCODE_1:
				nes->mod_emu_speed( 0.05 );
				break;
			case SDL_SCANCODE_2:
				nes->mod_emu_speed( -0.05 );
				break;
			case SDL_SCANCODE_3:
				nes->set_emu_speed( 1.0 );
				break;
			case SDL_SCANCODE_4:
				nes->DEBUG_PATTERNTABLE = !nes->DEBUG_PATTERNTABLE;
				nes->get_display()->set_show_window( nes->get_display()->get_pt_window(), nes->DEBUG_PATTERNTABLE );
				break;
			case SDL_SCANCODE_5:
				nes->DEBUG_NAMETABLE = !nes->DEBUG_NAMETABLE;
				nes->get_display()->set_show_window( nes->get_display()->get_nt_window(), nes->DEBUG_NAMETABLE );
				break;
			case SDL_SCANCODE_6:
				nes->DEBUG_APU = !nes->DEBUG_APU;
				nes->get_display()->set_show_window( nes->get_display()->get_apu_window(), nes->DEBUG_APU );
				break;
			default:
				break;
		}
	}
}

void UI::show_rom_dialog()
{
    nfdresult_t result = NFD_OpenDialog( "nes", nullptr, &outPath );

    if ( result == NFD_OKAY )
    {
        if ( nes->run( outPath ) )
        {
            state = UIState::PAUSE;
			cart_error = false;
		}
		else
		{
			state = UIState::MAIN;
			cart_error = true;
		}
		needs_update = true;
    }
}

void UI::draw()
{
	if ( needs_update )
	{
		SDL_SetRenderTarget( renderer_ui, texture_ui );
		SDL_RenderClear( renderer_ui );

		if ( state == UIState::PAUSE )
		{
			set_render_draw_color( Color::BLACK, 200 );
			SDL_RenderFillRect( renderer_ui, &screen_rect );
			draw_text( "PAUSED", 0, 0, 2, HAlign::LEFT, VAlign::TOP );

			for ( Button *b : pause_buttons )
			{
				if ( b->get_selected() )
				{
					draw_text( b->get_text(), b->get_x(), b->get_y(), 3,
					           HAlign::CENTER, VAlign::CENTER, Color::BLACK, Color::WHITE );
				}
				else
				{
					draw_text( b->get_text(), b->get_x(), b->get_y(), 3,
					           HAlign::CENTER, VAlign::CENTER );
				}
			}

		}
		else if ( state == UIState::MAIN )
		{
			set_render_draw_color( Color::BLACK, 255 );
			SDL_RenderFillRect( renderer_ui, &screen_rect );
			if ( show_splash )
			{
				SDL_Texture *splash_texture = SDL_CreateTextureFromSurface( renderer_ui, splash_img );
				SDL_Rect splash_rect{96 * 4, 75 * 4, 64 * 4, 80 * 4};
				SDL_RenderCopy( renderer_ui, splash_texture, nullptr, &splash_rect );
			}
			else
			{
				draw_text( "PRESS ENTER TO SELECT ROM", 512, 480, 2,
				           HAlign::CENTER, VAlign::CENTER );
				if ( cart_error )
				{
					std::string error_str = nes->get_cart()->get_error();
					draw_text( error_str, 512, 640, 1, HAlign::CENTER, VAlign::CENTER, Color::RED );
				}
			}
		}
		needs_update = false;
	}

	set_render_draw_color( Color::BLACK, 0 );
	SDL_SetRenderTarget( renderer_ui, texture_base );
	SDL_RenderCopy( renderer_ui, texture_ui, nullptr, nullptr );
}

void UI::draw_text( const std::string &text, int x, int y, float scale, HAlign h_align, VAlign v_align, Color col,
                    Color bgr_col )
{
	int text_w, text_h;

	SDL_Surface *text_surf = TTF_RenderText_Solid( font_ui, text.c_str(), colors[(int) col] );
	SDL_Texture *text_texture = SDL_CreateTextureFromSurface( renderer_ui, text_surf );

	TTF_SizeText( font_ui, text.c_str(), &text_w, &text_h );
	x -= text_w * scale / 2.0 * (int) h_align;
	y -= text_h * scale / 2.0 * (int) v_align;
	SDL_Rect text_rect{x, y, static_cast<int>(text_w * scale), static_cast<int>(text_h * scale)};

	if ( bgr_col != Color::NONE )
	{
		set_render_draw_color( bgr_col, 255 );
		SDL_Rect bgr_rect = text_rect;
		bgr_rect.x -= 4;
		bgr_rect.w += 4;
		SDL_RenderFillRect( renderer_ui, &bgr_rect );
	}
	SDL_RenderCopy( renderer_ui, text_texture, nullptr, &text_rect );
	SDL_FreeSurface( text_surf );
	SDL_DestroyTexture( text_texture );
}

void UI::set_render_draw_color( Color col, u8 alpha )
{
	SDL_SetRenderDrawColor( renderer_ui, colors[(int) col].r, colors[(int) col].g, colors[(int) col].b, alpha );
}