#pragma once

#include "SDL.h"
#include "../Component.h"
#include "Channel.h"
#include "Units.h"
#include <vector>

class APU : public Component
{
public:
	friend class FrameSequencer;

	APU();

	~APU();

	void cycle();

	void write_apu_reg( uint8_t reg, uint8_t data );

	void toggle_debug_mute( int channel );

private:
	void sample();

	float get_mixer();

public:
	SDL_AudioDeviceID audio_device;
private:
	SDL_AudioSpec audio_spec;

	Pulse pulse[2];
	Triangle triangle;
	Noise noise;
//    DMC dmc;
	FrameSequencer frameSeq;

	bool tick_fs = false;

	std::vector< int16_t > sample_buffer;
	float sample_clock = 0;

	static constexpr float sample_per = 21477272 / 44100.0 / 12.0;
};