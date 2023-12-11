#pragma once

#include <cstdint>
#include "Units.h"

class Channel {
public:
    Channel() : timer(0), sequencer(8) {};
    ~Channel() = default;
    void set_enabled(bool enable) { enabled = enable; if (!enable) { length = 0; length_halt = true; } }
    void set_timer_hi(uint8_t hi);
    void set_length_counter(uint8_t c);
    void set_timer_lo(uint8_t lo) { timer.set_lo(lo); }
    void set_length_halt(bool halt);
    void set_constant_vol(bool cv) { envelope.set_constant_vol(cv); }
    void set_vol(uint8_t v) { envelope.set_param(v); }
    void clock_length() { if (!length_halt && length > 0) length--; }
    void clock_env() { envelope.clock(); }
    void tick_timer();

    virtual uint8_t get_output() = 0;
protected:
    bool enabled = false;

    Divider timer;
    Sequencer sequencer;
    Envelope envelope;
    uint8_t seq_out = 0;

    uint8_t length = 0;
    bool length_halt = false;

    static constexpr uint8_t length_lookup[2][16] {
            {0x0A, 0x14, 0x28, 0x50, 0xA0, 0x3C, 0x0E, 0x1A,
             0x0C, 0x18, 0x30, 0x60, 0xC0, 0x48, 0x10, 0x20},
            {0xFE, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
             0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E}
    };
};

class Pulse : public Channel {
public:
    Pulse(bool p2) : Channel(), sweep_divider(1), p2(p2) { sequencer.steps = 8; sequencer.sequence = seqs[0]; };
    void set_duty(uint8_t duty);
    uint8_t get_output() override;

    void update_sweep(uint8_t byte);
    void tick_sweep();
private:
    static constexpr uint8_t seqs[4][8] = {
            {0, 0, 0, 0, 0, 0, 0, 1},
            {0, 0, 0, 0, 0, 0, 1, 1},
            {0, 0, 0, 0, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 0, 0}
    };

    Divider sweep_divider;
    bool sweep_enable = false;
    bool sweep_negate = false;
    bool sweep_reload = false;
    uint8_t sweep_shift = 0;
    uint16_t sweep_period = 0;
    bool p2;

    bool muted = false;
};

class Triangle : public Channel {
public:
    Triangle() : Channel() { sequencer.steps = 32; sequencer.sequence = seq; }
    void set_counter_reload_val(uint8_t val) { counter_reload_val = val;}
    void set_flag_linc_reload() { flag_linc_reload = true; }
    void tick_timer();
    void tick_lc();

    uint8_t get_output() override;
private:
    static constexpr uint8_t seq[32] = {
            15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    };

    uint8_t linear_counter = 0;
    uint8_t counter_reload_val = 0;

    bool flag_linc_reload = false;
};

class Noise : public Channel {
public:
    Noise() : Channel() { sequencer.steps = 1; }
    void set_mode(bool m) { mode = m; }
    void set_period(uint8_t p) { timer.set_period(periods[p%16]); }
    void tick_timer();

    uint8_t get_output() override;
private:
    static constexpr uint16_t periods[16] = {
            4, 8, 16, 32, 64, 96, 128, 160, 202,
            254, 380, 508, 762, 1016, 2034, 4068
    };

    bool mode;
    uint16_t shifter = 1;
};
class DMC : public Channel {};