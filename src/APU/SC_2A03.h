#pragma once

#include "../NES.h"
#include "SoundChip.h"

class SC_2A03 : public SoundChip
{
public:
	friend class FrameSequencer;
	friend class APU;

	Channel *get_channel( int channel ) override;

	void write_reg( u8 reg, u8 data ) override;

	void clock() override;

	float get_output() override;

	int get_channel_count() override
	{
		return 5;
	}

	std::string get_name() override
	{
		return "Ricoh 2A03";
	}

	std::string get_channel_name( int channel ) override
	{
		switch ( channel )
		{
			case 0:
			case 1:
				return "Pulse " + std::to_string( channel + 1 );
			case 2:
				return "Triangle";
			case 3:
				return "Noise";
			case 4:
				return "DPCM";
			default:
				return "";
		}
	}

	std::array<u8, 3> get_debug_base_color( int channel ) override
	{
		return { 255, 255, 255 };
	}

	std::array<u8, 3> get_debug_waveform_color( int channel ) override
	{
		switch ( channel )
		{
			case 0:
			case 1:
				return { 255, 127, 127 };
			case 2:
				return { 127, 255, 127 };
			case 3:
				return { 127, 127, 255 };
			case 4:
				return { 200, 200, 200 };
			default:
				return { 255, 255, 255 };
		}
	}

	bool is_waveform_complex( int channel ) override
	{
		return channel == 4;
	}

	float get_debug_damping( int channel ) override
	{
		return 1.0;
	}

	virtual std::string get_debug_note_name( int channel ) override;

private:
	Pulse pulse[ 2 ];
	Triangle triangle;
	Noise noise;
	DMC dmc;
	FrameSequencer frameSeq;

	bool tick_fs = false;
};