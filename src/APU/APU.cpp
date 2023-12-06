#include "APU.h"
#include "../CPU.h"
#include <math.h>

APU::APU() {
    SDL_zero(audio_spec);
    audio_spec.freq = 44100;
    audio_spec.format = AUDIO_S16SYS;
    audio_spec.channels = 1;
    audio_spec.samples = 1024;
    audio_spec.callback = nullptr;

    audio_device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);

    SDL_PauseAudioDevice(audio_device, 0);

    frameSeq = new FrameSequencer(this);
}

APU::~APU() {
    SDL_CloseAudioDevice(audio_device);
}

void APU::cycle() {
    if (tick_fs) {
        frameSeq->tick();
        pulse[0].tick_timer();
        pulse[1].tick_timer();
        tick_fs = false;
    } else tick_fs = true;
    triangle.tick_timer();
}

float APU::get_mixer() {
    float pulse_out = 0.65 * (pulse[0].get_output() + pulse[1].get_output());/*0.00752 **/
    float tri_out = 0.15 * triangle.get_output();
    float sample = pulse_out + tri_out;
    if (has_last) {
        int damp_at = 10;
        float diff = abs(sample - last_sample);
        if (diff > damp_at) {
            float factor = pow(0.95, diff-damp_at);
            sample = sample*factor+last_sample*(1-factor);
        }
    } else {
        last_sample = sample;
        has_last = true;
    }
    return sample;
}

void APU::write_apu_reg(uint8_t reg, uint8_t data) {
    int p2;
    switch(reg) {
        case 0x0:
        case 0x4:
            p2 = reg == 0x4 ? 1 : 0;
            pulse[p2].set_duty((data >> 6) & 0x3);
            pulse[p2].set_length_halt((data >> 5) & 0x1);
            pulse[p2].set_constant_vol((data >> 4) & 0x1);
            pulse[p2].set_vol(data & 0xF);
            break;
        case 0x1:
        case 0x5:
            p2 = reg == 0x5 ? 1 : 0;
            pulse[p2].update_sweep(data);
            break;
        case 0x2:
        case 0x6:
            p2 = reg == 0x6 ? 1 : 0;
            pulse[p2].set_timer_lo(data);
            break;
        case 0x3:
        case 0x7:
            p2 = reg == 0x7 ? 1 : 0;
            pulse[p2].set_timer_hi(data);
            break;
        case 0x8:
            triangle.set_length_halt((data >> 7) & 0x1);
            triangle.set_counter_reload_val(data & 0x7F);
            break;
        case 0xA:
            triangle.set_timer_lo(data);
            break;
        case 0xB:
            triangle.set_timer_hi(data);
            triangle.set_flag_linc_reload(true);
            break;
        case 0x15:
            pulse[0].set_enabled(data & 0x1);
            pulse[1].set_enabled((data >> 1) & 0x1);
            triangle.set_enabled((data >> 2) & 0x1);
            break;
        case 0x17:
            frameSeq->reset(data);
            break;
        default:
            break;
    }
}