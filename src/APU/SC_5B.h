#pragma once

#include "../NES.h"
#include "SoundChip.h"

class Square_5B : public Pulse
{
public:
	Square_5B() : Pulse()
	{
		init_lookup();
	}

	void set_timer_hi( u8 hi ) override
	{
		period = GET_BITS( period, 0, 8 ) | (GET_BITS( hi, 0, 4 ) << 8);
	}

	void set_timer_lo( u8 lo ) override
	{
		period = lo | (GET_BITS(period, 8, 4) << 8);
	}

	void tick_timer() override;

	void set_env_vol( u8 vol )
	{
		env_volume = vol;
	}

	u8 get_dac_in() override
	{
		return seq_out * env_volume;
	}

	float get_output() override;

	bool is_playing() override
	{
		return enabled;
	}

private:
	u8 clock_counter = 0;
	u16 period = 0;
	u16 timer = 0;
	u8 env_volume;

	static constexpr int DAC_STEPS = 32;
	float DAC_LOOKUP[ DAC_STEPS ];

	void init_lookup();
};

class SC_5B : public SoundChip
{
public:
	Channel *get_channel( int channel ) override;

	void write_reg( u8 reg, u8 data ) override;

	void clock() override;

	float get_output() override
	{
		return 0.0045 * (square[ 0 ].get_output() + square[ 1 ].get_output() + square[ 2 ].get_output());
	}

	int get_channel_count() override
	{
		return 3;
	}

	std::string get_name() override
	{
		return "Sunsoft 5B";
	}

	std::string get_channel_name( int channel ) override
	{
		if ( channel >= 0 && channel <= 2 )
		{
			return "Square " + std::to_string(channel + 1);
		}
		else
		{
			return "";
		}
	}

	std::array<u8, 3> get_debug_base_color( int channel ) override
	{
		return { 0, 127, 255 };
	}

	std::array<u8, 3> get_debug_waveform_color( int channel ) override
	{
		return { 0, 127, 255 };
	}

	float get_debug_damping( int channel ) override
	{
		return 2.0;
	}
private:
	Square_5B square[ 3 ];
};