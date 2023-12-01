#include "Display.h"
#include <SDL.h>
#include <windows.h>

bool Display::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("sdl2_pixelbuffer",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WIDTH*4,
                                          HEIGHT*4,
                                          SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        return false;
    }

    SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);

    texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGB24,
            SDL_TEXTUREACCESS_STREAMING,
            WIDTH,
            HEIGHT);
    if (texture == NULL) {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool Display::refresh() {
    int texture_pitch = 0;
    void* texture_pixels = NULL;
    if (SDL_LockTexture(texture, NULL, &texture_pixels, &texture_pitch) != 0) {
        SDL_Log("Unable to lock texture: %s", SDL_GetError());
    } else {
        memcpy(texture_pixels, pixels, texture_pitch * HEIGHT);
    }
    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    return true;
}

void Display::black() {
    for (int p = 0; p < WIDTH*HEIGHT*3; p+=3) {
        pixels[p] = 0;
        pixels[p+1] = 0;
        pixels[p+2] = 0;
    }
}

void Display::close() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Display::set_pixel_buffer(uint8_t x, uint8_t y, uint8_t rgb[3]) {
    buffer[(x+(y*WIDTH))*3] = rgb[0];
    buffer[(x+(y*WIDTH))*3+1] = rgb[1];
    buffer[(x+(y*WIDTH))*3+2] = rgb[2];
}

void Display::push_buffer() {
    black();
    for (int p = 0; p < WIDTH*HEIGHT*3; p ++) {
        pixels[p] = buffer[p];
        buffer[p] = 0;
    }
}
