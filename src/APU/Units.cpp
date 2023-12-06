#include "Units.h"
#include "APU.h"
#include "../CPU.h"

bool Divider::clock() {
    if (counter == 0) {
        counter = period;
        return true;
    } else {
        counter--;
        return false;
    }
}

void Divider::reload() { counter = period; }

uint8_t Sequencer::next() {
    uint8_t val = sequence != nullptr ? sequence[step] : step;
    if (step == 0) step = steps - 1;
    else step--;
    return val;
}

void Envelope::clock() {
    if (!start) {
        if (div.clock()) {
            div.set_period(param);
            div.reload();
            if (loop && decay == 0) decay = 15;
            else if (decay > 0) decay--;
        }
    } else {
        div.set_period(param);
        div.reload();
        start = false;
        decay = 15;
    }
}

void FrameSequencer::reset(uint8_t byte) {
    irq_disable = (byte >> 6) & 0x1;
    bool mode = (byte >> 7) & 0x1;
    if (!mode) sequencer.steps = 4;
    else {
        sequencer.steps = 5;
        do_seq(sequencer.next());
    }
}

void FrameSequencer::do_seq(uint8_t seq) {
    if (sequencer.steps == 4) {
        switch(seq) {
            case 3:
                set_interrupt();
            case 1:
                do_length_clock();
            case 0:
            case 2:
                do_env_clock();
            default:
                break;
        }
    } else {
        switch(seq) {
            case 0:
            case 2:
                do_length_clock();
            case 1:
            case 3:
                do_env_clock();
            case 4:
            default:
                break;
        }
    }
}

void FrameSequencer::set_interrupt() {
    if (!irq_disable) {
        apu->get_nes()->get_cpu()->trigger_irq();
    }
}

void FrameSequencer::do_length_clock() {
    apu->pulse[0].clock_length();
    apu->pulse[1].clock_length();
    apu->pulse[0].tick_sweep();
    apu->pulse[1].tick_sweep();
    apu->triangle.clock_length();
    apu->noise.clock_length();
}

void FrameSequencer::do_env_clock() {
    apu->pulse[0].clock_env();
    apu->pulse[1].clock_env();
    apu->triangle.tick_lc();
    apu->noise.clock_env();
}