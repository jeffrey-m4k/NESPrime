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
    void close();
    void black();
    void set_pixel_buffer(uint8_t x, uint8_t y, uint8_t rgb[3]);
    void push_buffer();
    uint8_t* get_pixels() { return pixels; }
private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    uint8_t pixels[WIDTH*HEIGHT*3] = {0};
    uint8_t buffer[WIDTH*HEIGHT*3] = {0};
};

#endif
