#include "UI.h"
#include "data.h"

using namespace std::filesystem;

bool UI::init() {
    texture_ui = nes->get_display()->get_sys_texture();
    texture_base = nes->get_display()->get_base_texture();
    renderer_ui = nes->get_display()->get_main_renderer();

    SDL_RWops* font_mem = SDL_RWFromConstMem(ui_font, sizeof(ui_font));
    font_ui = TTF_OpenFontRW(font_mem, 1, 24);
    if (font_ui == nullptr) {
        SDL_Log("Unable to initialize UI ui_font: %s", SDL_GetError());
        return false;
    }

    show = true;
    state = MAIN;

    pause_buttons.push_back(new Button("LOAD NEW ROM", 512, 720));
    pause_buttons.push_back(new Button("QUIT", 512, 800));
    pause_buttons.at(0)->set_selected(true);

    return true;
}

void UI::tick() {
    if (!show) return;

    if (state == MAIN) {
        if (!crawled) {
            for (recursive_directory_iterator i(".."), end; i != end; ++i)
                if (!is_directory(i->path()) && i->path().extension() == ".nes")
                    roms.push_back(i->path());
            if (!roms.empty()) {
                rom_select = roms.at(0);
                rom_select_idx = 0;
                found_roms = true;
            }
            crawled = true;
            needs_update = true;
        }
    }
}

void UI::handle(SDL_Event &e) {
    SDL_Scancode sym = e.key.keysym.scancode;
    switch(state) {
        case MAIN:
            if (found_roms) {
                switch (sym) {
                    case SDL_SCANCODE_RIGHT:
                        rom_select_idx++;
                        break;
                    case SDL_SCANCODE_LEFT:
                        rom_select_idx--;
                        break;
                    case SDL_SCANCODE_RETURN:
                        nes->run(rom_select.string());
                        show = false;
                        state = PAUSE;
                    default:
                        break;
                }
                if (rom_select_idx < 0) rom_select_idx = roms.size() - 1;
                else if (rom_select_idx >= roms.size()) rom_select_idx = 0;
                rom_select = roms.at(rom_select_idx);
                needs_update = true;
            }
            break;
        case PAUSE:
            switch (sym) {
                case SDL_SCANCODE_UP:
                case SDL_SCANCODE_DOWN:
                    for (int b = 0; b < pause_buttons.size(); ++b) {
                        Button* but = pause_buttons.at(b);
                        if (but->get_selected()) {
                            but->set_selected(false);
                            if (sym == SDL_SCANCODE_DOWN && ++b == pause_buttons.size()) pause_buttons.at(0)->set_selected(true);
                            else if (sym == SDL_SCANCODE_UP && --b < 0) pause_buttons.at(pause_buttons.size()-1)->set_selected(true);
                            else pause_buttons.at(b)->set_selected(true);
                        }
                    }
                    needs_update = true;
                    break;
                case SDL_SCANCODE_RETURN:
                    if (pause_buttons.at(0)->get_selected()) {
                        nes->reset();
                        state = MAIN;
                        needs_update = true;
                    } else if (pause_buttons.at(1)->get_selected()) {
                        nes->kill();
                    }
                default:
                    break;
            }
            break;
    }
    handle_global(e);
}

void UI::handle_global(SDL_Event &e) {
    if (e.key.keysym.scancode == SDL_SCANCODE_F11) {
        SDL_Window* win = nes->get_display()->get_main_window();
        if (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
            SDL_SetWindowFullscreen(win, 0);
            SDL_ShowCursor(SDL_ENABLE);
        } else {
            SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
            SDL_ShowCursor(SDL_DISABLE);
        }
    }
}

void UI::draw() {
    if (needs_update) {
        SDL_SetRenderTarget(renderer_ui, texture_ui);
        SDL_RenderClear(renderer_ui);

        if (state == PAUSE) {
            set_render_draw_color(BLACK, 200);
            SDL_RenderFillRect(renderer_ui, &screen_rect);
            draw_text("PAUSED", 0, 0, 2, H_LEFT, V_TOP);

            for (Button* b : pause_buttons) {
                if (b->get_selected()) draw_text(b->get_text(), b->get_x(), b->get_y(), 3, H_CENTER, V_CENTER, BLACK, WHITE);
                else draw_text(b->get_text(), b->get_x(), b->get_y(), 3, H_CENTER, V_CENTER);
            }

        } else if (state == MAIN) {
            set_render_draw_color(BLACK, 255);
            SDL_RenderFillRect(renderer_ui, &screen_rect);
            std::string text = found_roms ? "<- " + rom_select.filename().string() + " ->": "NO ROMS FOUND";
            draw_text(text, 512, 720, 2, H_CENTER, V_TOP);
            draw_text("NESPrime", 512, 320, 5, H_CENTER, V_BOTTOM);
            draw_text("Please select a ROM:", 512, 640, 1, H_CENTER, V_TOP);
        }
        needs_update = false;
    }

    set_render_draw_color(BLACK, 0);
    SDL_SetRenderTarget(renderer_ui, texture_base);
    SDL_RenderCopy(renderer_ui, texture_ui, nullptr, nullptr);
}

void UI::draw_text(const std::string& text, int x, int y, float scale, UIAlignmentH h_align, UIAlignmentV v_align, UIColor col, UIColor bgr_col) {
    int text_w, text_h;

    SDL_Surface* text_surf = TTF_RenderText_Solid(font_ui, text.c_str(), colors[col]);
    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer_ui, text_surf);

    TTF_SizeText(font_ui, text.c_str(), &text_w, &text_h);
    x -= text_w * scale / 2.0 * h_align;
    y -= text_h * scale / 2.0 * v_align;
    SDL_Rect text_rect{x, y, static_cast<int>(text_w*scale), static_cast<int>(text_h*scale)};

    if (bgr_col != NONE) {
        set_render_draw_color(bgr_col, 255);
        SDL_Rect bgr_rect = text_rect;
        bgr_rect.x -= 4; bgr_rect.w += 4;
        SDL_RenderFillRect(renderer_ui, &bgr_rect);
    }
    SDL_RenderCopy(renderer_ui, text_texture, nullptr, &text_rect);
    SDL_FreeSurface(text_surf);
    SDL_DestroyTexture(text_texture);
}

void UI::set_render_draw_color(UIColor col, uint8_t alpha) {
    SDL_SetRenderDrawColor(renderer_ui, colors[col].r, colors[col].g, colors[col].b, alpha);
}