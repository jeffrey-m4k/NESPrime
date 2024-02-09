#include <iostream>
#include "Channel.h"

void Channel::set_timer_hi( u8 hi )
{
	timer.set_hi( hi );
	set_length_counter( hi >> 3 );
}

void Channel::set_length_counter( u8 c )
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

void Pulse::set_timer_hi( u8 hi )
{
	Channel::set_timer_hi( hi );
	sequencer.reset();
}

void Pulse::set_duty( u8 duty )
{
	this->duty = duty;
	sequencer.sequence = seqs[duty];
}

void Pulse::update_sweep( u8 byte )
{
	sweep_enable = GET_BIT( byte, 7 );
	sweep_period = GET_BITS( byte, 4, 3 );
	sweep_negate = GET_BIT( byte, 3 );
	sweep_shift = GET_BITS( byte, 0, 3 );
	sweep_reload = true;
}

void Pulse::tick_sweep()
{
	u16 curr_period = timer.get_period();
	i16 change_amt = curr_period >> sweep_shift;
	if ( sweep_negate )
	{
		change_amt = -change_amt;
		if ( p2 )
		{
			change_amt++;
		}
	}
	u16 target_period = curr_period + (change_amt % 0x800);
	muted = curr_period < 8 || target_period > 0x7FF;

	bool zero = sweep_divider.clock();
	if ( zero && sweep_enable && !muted && sweep_shift != 0 )
	{
		timer.set_period( target_period );
	}
	if ( zero || sweep_reload )
	{
		sweep_divider.set_period( sweep_period );
	}
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

void Noise::tick_timer()
{
	if ( timer.clock() )
	{
		bool feedback = GET_BIT( shifter, 0 ) ^ (mode ? GET_BIT( shifter, 6 ) : GET_BIT( shifter, 1 ));
		shifter >>= 1;
		CLEAR_BIT( shifter, 15 );
		SET_BIT( shifter, 14, feedback );
	}
}

void DMC::tick_timer()
{
	if ( timer.clock() )
	{
		read_sample_next();
		tick_sample();
	}

	if ( irq_pending )
	{
		cpu->trigger_irq();
	}
}

// === DAC ===

u8 Pulse::get_dac_in()
{
	return seq_out * envelope.get_volume();
}

u8 Triangle::get_dac_in()
{
	return seq_out;
}

u8 Noise::get_dac_in()
{
	return !GET_BIT( shifter, 0 ) ? envelope.get_volume() : 0;
}

u8 DMC::get_dac_in()
{
	return output;
}

float Channel::get_output()
{
	if ( debug_muted ) return 0;

	u8 dac_in_curr = is_playing() ? get_dac_in() : dac_in_last;
	
	dac_out_last += (dac_in_curr - dac_in_last) / 15.0;
	dac_out_last *= 0.9999;

	dac_in_last = dac_in_curr;

	return dac_out_last;
}