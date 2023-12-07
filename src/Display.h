#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL.h>
#include "Component.h"

#define WIDTH 256
#define HEIGHT 240

class Display : public Component {
public:
    Display() { init(); };
    ~Display();
    bool init();
    bool refresh();
    bool update_pt();
    bool update_nt();
    void close();
    void black();
    void set_pixel_buffer(uint8_t x, uint8_t y, const uint8_t rgb[3]);
    void write_pt_pixel(uint8_t tile, uint8_t x, uint8_t y, bool pt2, const uint8_t rgb[3]);
    void write_nt_pixel(int tile, uint8_t x, uint8_t y, bool nt2, const uint8_t rgb[3]);
    void push_buffer();
    uint8_t* get_pixels() { return pixels; }
    void set_show_sys_texture(bool show);
    SDL_Renderer* get_main_renderer() { return renderer_main; }
    SDL_Texture* get_base_texture() { return texture_main_base; }
    SDL_Texture* get_sys_texture() { return texture_main_ui; }
    void set_sys_texture(SDL_Texture* tex) { texture_main_ui = tex; }
public:
    uint32_t last_update = 0;
private:
    SDL_Window *window_main;
    SDL_Window *window_pt;
    SDL_Window *window_nt;
    SDL_Renderer *renderer_main;
    SDL_Renderer *renderer_pt;
    SDL_Renderer *renderer_nt;
    SDL_Texture *texture_game;
    SDL_Texture *texture_pt;
    SDL_Texture *texture_nt;

    uint8_t pixels[WIDTH*HEIGHT*3] = {0};
    uint8_t buffer[WIDTH*HEIGHT*3] = {0};

    uint8_t pt[256*128*3] = {0};
    uint8_t nts[256*240*2*3] = {0};

    uint32_t fps_lasttime;
    uint32_t fps_current;
    uint32_t fps_frames;

    SDL_Texture* texture_main_base;
    SDL_Texture* texture_main_ui = nullptr;
    bool show_sys_texture = false;
};

#endif
