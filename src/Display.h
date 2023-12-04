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
private:
    SDL_Window *window_main;
    SDL_Window *window_pt;
    SDL_Window *window_nt;
    SDL_Renderer *renderer_main;
    SDL_Renderer *renderer_pt;
    SDL_Renderer *renderer_nt;
    SDL_Texture *texture_main;
    SDL_Texture *texture_pt;
    SDL_Texture *texture_nt;

    uint8_t pixels[WIDTH*HEIGHT*3] = {0};
    uint8_t buffer[WIDTH*HEIGHT*3] = {0};

    uint8_t pt[256*128*3] = {0};
    uint8_t nts[256*240*2*3] = {0};
};

#endif
