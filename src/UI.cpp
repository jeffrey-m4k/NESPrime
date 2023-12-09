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
    if (state == MAIN) {
        if (found_roms) {
            switch (e.key.keysym.scancode) {
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
    }
}

void UI::draw() {
    if (needs_update) {
        SDL_SetRenderTarget(renderer_ui, texture_ui);
        SDL_RenderClear(renderer_ui);

        if (state == PAUSE) {
            set_render_draw_color(BLACK, 127);
            SDL_RenderFillRect(renderer_ui, &screen_rect);
            draw_text("PAUSED", 0, 0, 2, H_LEFT, V_TOP);
        } else if (state == MAIN) {
            set_render_draw_color(WHITE, 255);
            SDL_Rect rect {8, 8, 1007, 943};
            SDL_RenderDrawRect(renderer_ui, &rect);
            rect = {9, 9, 1005, 941};
            SDL_RenderDrawRect(renderer_ui, &rect);
            rect = {10, 10, 1003, 939};
            SDL_RenderDrawRect(renderer_ui, &rect);
            rect = {11, 11, 1001, 937};
            SDL_RenderDrawRect(renderer_ui, &rect);
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

void UI::draw_text(std::string text, int x, int y, float scale, UIAlignmentH h_align, UIAlignmentV v_align) {
    int text_w, text_h;

    SDL_Surface* text_surf = TTF_RenderText_Solid(font_ui, text.c_str(), colors[WHITE]);
    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer_ui, text_surf);

    TTF_SizeText(font_ui, text.c_str(), &text_w, &text_h);
    x -= text_w * scale / 2.0 * h_align;
    y -= text_h * scale / 2.0 * v_align;
    SDL_Rect text_rect{x, y, static_cast<int>(text_w*scale), static_cast<int>(text_h*scale)};

    SDL_RenderCopy(renderer_ui, text_texture, nullptr, &text_rect);
    SDL_FreeSurface(text_surf);
    SDL_DestroyTexture(text_texture);
}

void UI::set_render_draw_color(UIColor col, uint8_t alpha) {
    SDL_SetRenderDrawColor(renderer_ui, colors[col].r, colors[col].g, colors[col].b, alpha);
}