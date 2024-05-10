#include "SC_5B.h"

void Square_5B::tick_timer()
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

float Square_5B::get_output()
{
	u8 dac_in_curr = is_playing() ? get_dac_in() : dac_in_last;

	dac_out_last += (DAC_LOOKUP[ dac_in_curr ] - DAC_LOOKUP[ dac_in_last ]) / 32.0;
	dac_out_last *= 0.9999;

	dac_in_last = dac_in_curr;

	return debug_muted ? 0 : dac_out_last;
}

void Square_5B::init_lookup()
{
	for ( int i = 0; i < DAC_STEPS; ++i )
	{
		float dB = 1.5 * pow( i, 1.3 );
		DAC_LOOKUP[ i ] = pow( 10, dB / 20 );
	}
}

Channel *SC_5B::get_channel( int channel )
{
	channel %= 3;
	if ( channel < 0 ) channel += 3;
	return &square[ channel ];
}

void SC_5B::write_reg( u8 reg, u8 data )
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

void SC_5B::clock()
{
	for ( int i = 0; i < 3; ++i )
	{
		square[ i ].tick_timer();
	}
}