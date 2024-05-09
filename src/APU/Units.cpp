#include "Units.h"
#include "APU.h"
#include "../CPU.h"

bool Divider::clock()
{
	if ( counter == 0 )
	{
		counter = period;
		return true;
	}
	else
	{
		counter--;
		return false;
	}
}

void Divider::reload()
{
	counter = period;
}

u8 Sequencer::next()
{
	u8 val = sequence != nullptr ? sequence[step] : step;
	if ( step == 0 )
	{
		step = steps - 1;
	}
	else
	{
		step--;
	}
	return val;
}

void Envelope::clock()
{
	if ( !start )
	{
		if ( div.clock() )
		{
			div.set_period( param );
			div.reload();
			if ( loop && decay == 0 )
			{
				decay = 15;
			}
			else if ( decay > 0 )
			{
				decay--;
			}
		}
	}
	else
	{
		div.set_period( param );
		div.reload();
		start = false;
		decay = 15;
	}
}

void FrameSequencer::reset( u8 byte )
{
	irq_disable = GET_BIT( byte, 6 );
	if ( irq_disable )
	{
		interrupt = false;
	}

	bool mode = GET_BIT( byte, 7 );
	if ( !mode )
	{
		sequencer.steps = 4;
	}
	else
	{
		sequencer.steps = 5;
		do_seq( sequencer.next() );
	}
}

void FrameSequencer::do_seq( u8 seq )
{
	if ( sequencer.steps == 4 )
	{
		switch ( seq )
		{
			case 3:
				set_interrupt();
			case 1:
				do_length_clock();
			case 0:
			case 2:
				do_env_clock();
			default:
				break;
		}
	}
	else
	{
		switch ( seq )
		{
			case 0:
			case 2:
				do_length_clock();
			case 1:
			case 3:
				do_env_clock();
			case 4:
			default:
				break;
		}
	}
}

void FrameSequencer::set_interrupt()
{
	if ( !irq_disable )
	{
		interrupt = true;
	}
}

void FrameSequencer::do_length_clock()
{
	sc->pulse[0].clock_length();
	sc->pulse[1].clock_length();
	sc->pulse[0].tick_sweep();
	sc->pulse[1].tick_sweep();
	sc->triangle.clock_length();
	sc->noise.clock_length();
}

void FrameSequencer::do_env_clock()
{
	sc->pulse[0].clock_env();
	sc->pulse[1].clock_env();
	sc->triangle.tick_lc();
	sc->noise.clock_env();
}