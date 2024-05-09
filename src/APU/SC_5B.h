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

	void tick_timer() override
	{
		if ( ++clock_counter == 16 )
		{
			clock_counter = 0;
			if ( timer == 0 || --timer == 0 )
			{
				timer = period;
				seq_out = (seq_out == 1) ? 0 : 1;
			}
		}
	}

	void set_env_vol( u8 vol )
	{
		env_volume = vol;
	}

	u8 get_dac_in() override
	{
		return seq_out * env_volume;
	}

	float get_output() override
	{
		u8 dac_in_curr = is_playing() ? get_dac_in() : dac_in_last;

		dac_out_last += (DAC_LOOKUP[ dac_in_curr ] - DAC_LOOKUP[ dac_in_last ]) / 32.0;
		dac_out_last *= 0.9999;

		dac_in_last = dac_in_curr;

		return debug_muted ? 0 : dac_out_last;
	}

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

	void init_lookup()
	{
		for ( int i = 0; i < DAC_STEPS; ++i )
		{
			float dB = 1.5 * pow(i, 1.3);
			DAC_LOOKUP[ i ] = pow( 10, dB / 20 );
		}
	}
};

class SC_5B : public SoundChip
{
public:
	Channel *get_channel( int channel ) override
	{
		channel %= 3;
		if ( channel < 0 ) channel += 3;
		return &square[ channel ];
	}

	void write_reg( u8 reg, u8 data ) override
	{
		switch ( reg )
		{
			case 0x0: 
			case 0x2: 
			case 0x4:
				square[ reg / 0x2 ].set_timer_lo( data );
				break;
			case 0x1: 
			case 0x3: 
			case 0x5:
				square[ reg / 0x2 ].set_timer_hi( data );
				break;
			case 0x7:
				for ( int i = 0; i < 3; ++i )
				{
					square[ i ].set_enabled( !GET_BIT( data, i ) );
				}
				break;
			case 0x8:
			case 0x9:
			case 0xA:
				square[ (reg - 0x8) ].set_env_vol( GET_BITS( data, 0, 4 ) );
				break;
			default:
				break;
		}
	}

	void clock() override
	{
		for ( int i = 0; i < 3; ++i )
		{
			square[ i ].tick_timer();
		}
	}

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