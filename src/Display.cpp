#include "Display.h"
#include "PPU.h"
#include <SDL.h>
#include <windows.h>

bool Display::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return false;
    }

    window_pt = SDL_CreateWindow("sdl2_pixelbuffer", 16, 16, 1024,512, SDL_WINDOW_RESIZABLE);
    window_nt = SDL_CreateWindow("sdl2_pixelbuffer", 16, 544, 1024,480, SDL_WINDOW_RESIZABLE);
    window_main = SDL_CreateWindow("sdl2_pixelbuffer",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                          WIDTH*4,
                                          HEIGHT*4,
                                   SDL_WINDOW_RESIZABLE);

    if (window_main == nullptr || window_pt == nullptr || window_nt == nullptr) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return false;
    }



    renderer_main = SDL_CreateRenderer(window_main, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    renderer_pt = SDL_CreateRenderer(window_pt, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    renderer_nt = SDL_CreateRenderer(window_nt, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer_main == nullptr || renderer_pt == nullptr || renderer_nt == nullptr) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        return false;
    }

    SDL_RenderSetLogicalSize(renderer_main, WIDTH, HEIGHT);
    SDL_RenderSetLogicalSize(renderer_pt, 256, 128);
    SDL_RenderSetLogicalSize(renderer_nt, 512, 240);

    texture_main = SDL_CreateTexture(renderer_main, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    texture_pt = SDL_CreateTexture(renderer_pt, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 128);
    texture_nt = SDL_CreateTexture(renderer_nt, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 512, 240);
    if (texture_main == nullptr || texture_pt == nullptr || texture_nt == nullptr) {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        return false;
    }

    fps_lasttime = SDL_GetTicks();
    fps_current = 0;
    fps_frames = 0;

    return true;
}

bool Display::update_pt() {
    int texture_pitch = 0;
    void* texture_pixels = nullptr;
    if (SDL_LockTexture(texture_pt, nullptr, &texture_pixels, &texture_pitch) == 0) {
        memcpy(texture_pixels, pt, texture_pitch * 128);
    }
    SDL_UnlockTexture(texture_pt);

    SDL_RenderClear(renderer_pt);
    SDL_RenderCopy(renderer_pt, texture_pt, nullptr, nullptr);
    SDL_RenderPresent(renderer_pt);

    return true;
}

bool Display::update_nt() {
    int texture_pitch = 0;
    void* texture_pixels = nullptr;
    if (SDL_LockTexture(texture_nt, nullptr, &texture_pixels, &texture_pitch) == 0) {
        memcpy(texture_pixels, nts, texture_pitch * 240);
    }
    SDL_UnlockTexture(texture_nt);

    SDL_RenderClear(renderer_nt);
    SDL_RenderCopy(renderer_nt, texture_nt, nullptr, nullptr);
    SDL_RenderPresent(renderer_nt);

    return true;
}

bool Display::refresh() {
    int texture_pitch = 0;
    void *texture_pixels = nullptr;
    if (SDL_LockTexture(texture_main, nullptr, &texture_pixels, &texture_pitch) != 0) {
        SDL_Log("Unable to lock texture: %s", SDL_GetError());
    } else {
        memcpy(texture_pixels, pixels, texture_pitch * HEIGHT);
    }
    SDL_UnlockTexture(texture_main);

    SDL_RenderClear(renderer_main);
    SDL_RenderCopy(renderer_main, texture_main, nullptr, nullptr);
    SDL_RenderPresent(renderer_main);

    fps_frames++;
    if (fps_lasttime < SDL_GetTicks() - 1000) {
        fps_lasttime = SDL_GetTicks();
        fps_current = fps_frames;
        fps_frames = 0;
    }
    SDL_SetWindowTitle(window_main, ("FPS:" + std::to_string(fps_current)).c_str());

    last_update = SDL_GetTicks();

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
    SDL_DestroyTexture(texture_main);
    SDL_DestroyTexture(texture_pt);
    SDL_DestroyTexture(texture_nt);
    SDL_DestroyRenderer(renderer_main);
    SDL_DestroyRenderer(renderer_pt);
    SDL_DestroyRenderer(renderer_nt);
    SDL_DestroyWindow(window_main);
    SDL_DestroyWindow(window_pt);
    SDL_DestroyWindow(window_nt);
    SDL_Quit();
}

void Display::set_pixel_buffer(uint8_t x, uint8_t y, const uint8_t rgb[3]) {
    buffer[(x+(y*WIDTH))*3] = rgb[0];
    buffer[(x+(y*WIDTH))*3+1] = rgb[1];
    buffer[(x+(y*WIDTH))*3+2] = rgb[2];
}

void Display::push_buffer() {
    //black();
    for (int p = 0; p < WIDTH*HEIGHT*3; p ++) {
        pixels[p] = buffer[p];
        buffer[p] = 0;
    }
}

void Display::write_pt_pixel(uint8_t tile, uint8_t x, uint8_t y, bool pt2, const uint8_t rgb[3]) {
    int index = (128*pt2 + 256*8*(tile/16) + 8*(tile%16) + x + 256*y) * 3;
    pt[index] = rgb[0];
    pt[index+1] = rgb[1];
    pt[index+2] = rgb[2];
}

void Display::write_nt_pixel(int tile, uint8_t x, uint8_t y, bool nt2, const uint8_t *rgb) {
    int index = (256*nt2 + 512*8*(tile/32) + 8*(tile%32) + x + 512*y) * 3;
    nts[index] = rgb[0];
    nts[index+1] = rgb[1];
    nts[index+2] = rgb[2];
}