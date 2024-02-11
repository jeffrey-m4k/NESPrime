#pragma once

#include "SDL.h"
#include "../Component.h"
#include "Channel.h"
#include "Units.h"
#include <vector>
#include <deque>

class APU : public Component
{
public:
	friend class FrameSequencer;

	APU();

	~APU();

	void init();

	void cycle();

	void write_apu_reg( u8 reg, u8 data );

	u8 read_status();

	void set_debug_mute( bool mute, int channel );

	void toggle_debug_mute( int channel );

	bool is_playing( int channel );

private:
	void sample();

	float get_mixer();

	void low_pass();

	void downsample();

	Channel *get_channel( int channel );

public:
	SDL_AudioDeviceID audio_device;
private:
	SDL_AudioSpec audio_spec;

	Pulse pulse[2];
	Triangle triangle;
	Noise noise;
    DMC dmc;
	FrameSequencer frameSeq;

	bool tick_fs = false;

	std::deque< float > sample_buffer_raw;
	std::vector< float > sample_buffer;
	float low_pass_last = 0;
	float sample_clock = 0;

	static constexpr float SAMPLE_RATE = 44100.0;
	static constexpr float sample_per = 21477272 / SAMPLE_RATE / 12.0;
};