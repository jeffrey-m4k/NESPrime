#pragma once

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
};