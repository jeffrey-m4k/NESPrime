#pragma once

#include "SDL.h"
#include "../Component.h"
#include "Channel.h"
#include "Units.h"
#include <vector>
#include <deque>

#include "SC_2A03.h"
#include "SC_5B.h"

class APU : public Component
{
public:
	APU();

	~APU();

	void init();

	void cycle();

	void write_apu_reg( u8 reg, u8 data );

	u8 read_status();

	SoundChip *get_chip( SCType type );

private:
	void sample();

	float get_mixer();

	void low_pass();

	void downsample();

public:
	SDL_AudioDeviceID audio_device;
private:
	SDL_AudioSpec audio_spec;

	std::deque< float > sample_buffer_raw;
	std::vector< float > sample_buffer;
	float low_pass_last = 0;
	float sample_clock = 0;

	static constexpr float SAMPLE_RATE = 44100.0;
	static constexpr float sample_per = 21477272 / SAMPLE_RATE / 12.0;

	// === SOUND CHIPS ===
	SC_2A03 sc_2a03;
	SC_5B sc_5b;
};