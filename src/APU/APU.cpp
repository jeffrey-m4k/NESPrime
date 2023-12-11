#include "APU.h"
#include "../CPU.h"

APU::APU() {
    SDL_setenv("SDL_AUDIODRIVER", "directsound", 1);
    SDL_zero(audio_spec);
    audio_spec.freq = 44100;
    audio_spec.format = AUDIO_S16SYS;
    audio_spec.channels = 1;
    audio_spec.samples = 1024;
    audio_spec.callback = nullptr;

    audio_device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);

    SDL_PauseAudioDevice(audio_device, 0);

    frameSeq.set_apu(this);
}

APU::~APU() {
    SDL_CloseAudioDevice(audio_device);
}

void APU::cycle() {
    if (tick_fs) {
        frameSeq.tick();
        pulse[0].tick_timer();
        pulse[1].tick_timer();
        noise.tick_timer();
        tick_fs = false;
    } else tick_fs = true;
    triangle.tick_timer();
    sample();
}

void APU::sample() {
    if (++sample_clock >= sample_per) {
        sample_buffer.push_back(get_mixer() * 500);
        sample_clock -= sample_per;
    }
    if (sample_buffer.size() >= 100) {
        SDL_QueueAudio(audio_device, sample_buffer.data(), sample_buffer.size() * 2);
        sample_buffer.clear();
        while (SDL_GetQueuedAudioSize(audio_device) > 4096 * 2) {};
    }
}

float APU::get_mixer() {
    float sample =
            0.752 * (pulse[0].get_output() + pulse[1].get_output()) +
            0.851 * triangle.get_output() +
            0.494 * noise.get_output();

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
            triangle.set_flag_linc_reload();
            break;
        case 0xC:
            noise.set_length_halt((data >> 5) & 0x1);
            noise.set_constant_vol((data >> 4) & 0x1);
            noise.set_vol(data & 0xF);
            break;
        case 0xE:
            noise.set_mode((data >> 7) & 0x1);
            noise.set_period(data & 0xF);
            break;
        case 0xF:
            noise.set_length_counter(data >> 3);
            break;
        case 0x15:
            pulse[0].set_enabled(data & 0x1);
            pulse[1].set_enabled((data >> 1) & 0x1);
            triangle.set_enabled((data >> 2) & 0x1);
            noise.set_enabled((data >> 3) & 0x1);
            break;
        case 0x17:
            frameSeq.reset(data);
            break;
        default:
            break;
    }
}