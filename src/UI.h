#ifndef UI_H
#define UI_H

#include "Component.h"
#include "Display.h"
#include "SDL_ttf.h"
#include <filesystem>
#include <vector>

enum UIState {
    MAIN, PAUSE
};

enum UIColor {
    WHITE, BLACK, RED
};

enum UIAlignmentH {
    H_LEFT, H_CENTER, H_RIGHT
};

enum UIAlignmentV {
    V_TOP, V_CENTER, V_BOTTOM
};

class UI : public Component {
public:
    bool init();
    void tick();
    void draw();
    void handle(SDL_Event& e);
    void set_show(bool s) { show = s; needs_update = s; nes->get_display()->set_show_sys_texture(s); }
    bool get_show() const { return show; }
    void set_state(UIState s) { state = s; needs_update = true; }
    UIState get_state() const { return state; }
private:
    void set_render_draw_color(UIColor col, uint8_t alpha);
    void draw_text(std::string text, int x, int y, float scale, UIAlignmentH h_align, UIAlignmentV v_align);
private:
    bool show = false;
    bool needs_update = true;
    UIState state = MAIN;

    SDL_Texture* texture_ui;
    SDL_Texture* texture_base;
    SDL_Renderer* renderer_ui;
    TTF_Font* font_ui;

    static constexpr SDL_Rect screen_rect {0, 0, WIDTH*4, HEIGHT*4};
    static constexpr SDL_Color colors[] = {
            {255, 255, 255},
            {0, 0, 0},
            {255, 0, 0}
    };

    // === STATE: MAIN
    std::vector<std::filesystem::path> roms;
    std::filesystem::path rom_select;
    int rom_select_idx;
    bool crawled = false;
    bool found_roms = false;
};


#endif
