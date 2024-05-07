#pragma once

#include <array>
#include "Channel.h"
#include "../BitUtils.h"

class ExpansionChip
{
public:
	virtual Channel* get_channel( int channel ) = 0;

	float peek_output( int channel )
	{
		return get_channel( channel )->peek_output();
	}

	virtual float get_output() = 0;

	virtual void write_reg( u8 reg, u8 data ) = 0;

	virtual void clock() = 0;

	virtual int get_channel_count() = 0;

	virtual std::string get_name() = 0;

	virtual std::string get_channel_name( int channel ) = 0;

	virtual std::array<u8, 3> get_debug_base_color() = 0;

	virtual std::array<u8, 3> get_debug_waveform_color() = 0;

	virtual float get_debug_damping( int channel ) = 0;
};