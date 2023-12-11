#include <iostream>
#include "Channel.h"

void Channel::set_timer_hi( uint8_t hi )
{
	timer.set_hi( hi );
	set_length_counter( hi >> 3 );
	sequencer.reset();
}

void Channel::set_length_counter( uint8_t c )
{
	length = length_lookup[c & 0x1][c >> 1];
	envelope.restart();
}

void Channel::set_length_halt( bool halt )
{
	length_halt = halt;
	envelope.set_loop( halt );
}

void Channel::tick_timer()
{
	if ( timer.clock() )
	{
		seq_out = sequencer.next();
	}
}

void Pulse::set_duty( uint8_t duty )
{
	sequencer.sequence = seqs[duty];
}

void Pulse::update_sweep( uint8_t byte )
{
	sweep_enable = (byte >> 7) & 0x1;
	sweep_period = (byte >> 4) & 0x7;
	sweep_negate = (byte >> 3) & 0x1;
	sweep_shift = byte & 0x7;
	sweep_reload = true;
}

void Pulse::tick_sweep()
{
	uint16_t curr_period = timer.get_period();
	uint16_t change_amt = curr_period >> sweep_shift;
	if ( sweep_negate )
	{
		change_amt = ~change_amt;
		if ( p2 )
		{
			change_amt++;
		}
	}
	uint16_t target_period = curr_period + change_amt;
	muted = curr_period < 8 || target_period > 0x7FF;

	bool zero = sweep_divider.clock();
	if ( zero && sweep_enable && !muted )
	{
		timer.set_period( target_period );
	}
	if ( zero || sweep_reload )
	{
		sweep_divider.set_period( sweep_period );
	}
}

uint8_t Pulse::get_output()
{
	return (enabled && timer.get_period() > 8 && !muted && length > 0) ? seq_out * envelope.get_volume() : 0;
}

void Triangle::tick_timer()
{
	if ( timer.clock() && length > 0 && linear_counter > 0 )
	{
		seq_out = sequencer.next();
	}
}

void Triangle::tick_lc()
{
	if ( flag_linc_reload )
	{
		linear_counter = counter_reload_val;
	}
	else if ( linear_counter > 0 )
	{
		linear_counter--;
	}

	if ( !length_halt )
	{
		flag_linc_reload = false;
	}
}

uint8_t Triangle::get_output()
{
	return (enabled && timer.get_period() > 8 && (length > 0 && linear_counter > 0 || length_halt)) ? seq_out : 0;
}

void Noise::tick_timer()
{
	if ( timer.clock() )
	{
		bool feedback = (shifter & 0x1) ^ (mode ? ((shifter >> 6) & 0x1) : ((shifter >> 1) & 0x1));
		shifter = (shifter >> 1) & ~0x8000;
		shifter |= (feedback << 14);
	}
}

uint8_t Noise::get_output()
{
	return (enabled && timer.get_period() > 8 && length > 0 && !(shifter & 0x1)) ? envelope.get_volume() : 0;
}