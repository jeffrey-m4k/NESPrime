#ifndef APU_H
#define APU_H

#include "SDL.h"
#include "../Component.h"
#include "Channel.h"
#include "Units.h"

class APU : public Component {
public:
    friend class FrameSequencer;
    APU();
    ~APU();
    void cycle();
    void write_apu_reg(uint8_t reg, uint8_t data);
    float get_mixer();
public:
    SDL_AudioDeviceID audio_device;
private:
    SDL_AudioSpec audio_spec;

    Pulse pulse[2] = {new Pulse(false), new Pulse(true)};
    Triangle triangle;
//    Noise noise;
//    DMC dmc;
    FrameSequencer* frameSeq;

    bool tick_fs = false;

    float last_sample = 0;
    bool has_last = false;
};

#endif