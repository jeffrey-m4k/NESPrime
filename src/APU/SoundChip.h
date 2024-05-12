#pragma once

#include <array>
#include <algorithm>
#include "Channel.h"
#include "../BitUtils.h"

class SoundChip
{
public:
	SoundChip();

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

	virtual std::array<u8, 3> get_debug_base_color( int channel ) = 0;

	virtual std::array<u8, 3> get_debug_waveform_color( int channel ) = 0;

	virtual bool is_waveform_complex( int channel ) = 0;

	virtual float get_debug_damping( int channel ) = 0;

	virtual std::string get_debug_note_name( int channel ) = 0;

protected:
	std::map<double, std::string> note_freqs;

	std::string freq_to_note( double freq );
};