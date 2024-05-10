#include "SC_2A03.h"

Channel *SC_2A03::get_channel( int channel )
{
	switch ( channel )
	{
		case 0:
		case 1:
			return &pulse[ channel ];
		case 2:
			return &triangle;
		case 3:
			return &noise;
		case 4:
			return &dmc;
		default:
			return nullptr;
	}
}

void SC_2A03::write_reg( u8 reg, u8 data )
{
	int p2;
	switch ( reg )
	{
		case 0x0:
		case 0x4:
			p2 = reg == 0x4 ? 1 : 0;
			pulse[ p2 ].set_duty( GET_BITS( data, 6, 2 ) );
			pulse[ p2 ].set_length_halt( GET_BIT( data, 5 ) );
			pulse[ p2 ].set_constant_vol( GET_BIT( data, 4 ) );
			pulse[ p2 ].set_vol( GET_BITS( data, 0, 4 ) );
			break;
		case 0x1:
		case 0x5:
			p2 = reg == 0x5 ? 1 : 0;
			pulse[ p2 ].update_sweep( data );
			break;
		case 0x2:
		case 0x6:
			p2 = reg == 0x6 ? 1 : 0;
			pulse[ p2 ].set_timer_lo( data );
			break;
		case 0x3:
		case 0x7:
			p2 = reg == 0x7 ? 1 : 0;
			pulse[ p2 ].set_timer_hi( data );
			break;
		case 0x8:
			triangle.set_length_halt( GET_BIT( data, 7 ) );
			triangle.set_counter_reload_val( GET_BITS( data, 0, 7 ) );
			break;
		case 0xA:
			triangle.set_timer_lo( data );
			break;
		case 0xB:
			triangle.set_timer_hi( data );
			triangle.set_flag_linc_reload();
			break;
		case 0xC:
			noise.set_length_halt( GET_BIT( data, 5 ) );
			noise.set_constant_vol( GET_BIT( data, 4 ) );
			noise.set_vol( GET_BITS( data, 0, 4 ) );
			break;
		case 0xE:
			noise.set_mode( GET_BIT( data, 7 ) );
			noise.set_period( GET_BITS( data, 0, 4 ) );
			break;
		case 0xF:
			noise.set_length_counter( GET_BITS( data, 3, 5 ) );
			break;
		case 0x10:
			dmc.set_status( data );
			break;
		case 0x11:
			dmc.set_output( data );
			break;
		case 0x12:
			dmc.set_sample_address( data );
			break;
		case 0x13:
			dmc.set_sample_length( data );
			break;
		case 0x15:
			pulse[ 0 ].set_enabled( GET_BIT( data, 0 ) );
			pulse[ 1 ].set_enabled( GET_BIT( data, 1 ) );
			triangle.set_enabled( GET_BIT( data, 2 ) );
			noise.set_enabled( GET_BIT( data, 3 ) );
			dmc.set_enabled( GET_BIT( data, 4 ) );
			dmc.clear_interrupt();
			break;
		case 0x17:
			frameSeq.reset( data );
			break;
		default:
			break;
	}
}

void SC_2A03::clock()
{
	if ( tick_fs )
	{
		frameSeq.tick();
		pulse[ 0 ].tick_timer();
		pulse[ 1 ].tick_timer();
		noise.tick_timer();
		dmc.tick_timer();
		tick_fs = false;
	}
	else
	{
		tick_fs = true;
	}
	triangle.tick_timer();
}

float SC_2A03::get_output()
{
	return
		0.00752 * (pulse[ 0 ].get_output() + pulse[ 1 ].get_output()) +
		0.00851 * triangle.get_output() +
		0.00494 * noise.get_output() +
		0.00285 * dmc.get_output();
}