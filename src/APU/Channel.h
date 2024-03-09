#pragma once

#include <map>
#include <cmath>
#include <random>
#include "Units.h"
#include "../CPU.h"

class Channel
{
public:
	Channel() : timer( 0 ), sequencer( 8 )
	{
	};

	~Channel() = default;

	virtual void set_enabled( bool enable )
	{
		enabled = enable;
		if ( !enable )
		{
			length = 0;
			length_halt = true;
		}
	}

	virtual void set_timer_hi( u8 hi );

	void set_length_counter( u8 c );

	virtual void set_timer_lo( u8 lo )
	{
		timer.set_lo( lo );
	}

	void set_length_halt( bool halt );

	void set_constant_vol( bool cv )
	{
		envelope.set_constant_vol( cv );
	}

	void set_vol( u8 v )
	{
		envelope.set_param( v );
	}

	bool get_length_halt()
	{
		return length_halt;
	}

	void clock_length()
	{
		if ( !length_halt && length > 0 )
		{
			length--;
		}
	}

	void clock_env()
	{
		envelope.clock();
	}

	virtual void tick_timer();

	void toggle_debug_mute()
	{
		debug_muted = !debug_muted;
	}

	virtual bool is_playing() = 0;

	virtual u8 get_dac_in() = 0;

	virtual float get_output();

	float peek_output();

public:
	bool debug_muted = false;

protected:
	bool enabled = false;

	Divider timer;
	Sequencer sequencer;
	Envelope envelope;
	u8 seq_out = 0;

	u8 dac_in_last = 0;
	float dac_out_last = 0.0;

	u8 length = 0;
	bool length_halt = false;

	static constexpr u8 length_lookup[2][16]{
			{
					0x0A, 0x14, 0x28, 0x50, 0xA0, 0x3C, 0x0E, 0x1A,
					0x0C, 0x18, 0x30, 0x60, 0xC0, 0x48, 0x10, 0x20
			},
			{
					0xFE, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
					0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E
			}
	};
};

class Pulse : public Channel
{
public:
	Pulse() : Pulse( false ) {}

	Pulse( bool p2 ) : Channel(), sweep_divider( 1 ), p2( p2 )
	{
		sequencer.steps = 8;
		sequencer.sequence = seqs[0];
	};

	void set_timer_hi( u8 hi ) override;

	void set_duty( u8 duty );

	void set_p2( bool is_p2 )
	{
		p2 = is_p2;
	}

	u8 get_dac_in() override;

	void update_sweep( u8 byte );

	void tick_sweep();

	bool is_playing() override
	{
		return enabled && timer.get_period() > 8 && !muted && length > 0;
	}

private:
	static constexpr u8 seqs[4][8] = {
			{0, 0, 0, 0, 0, 0, 0, 1},
			{0, 0, 0, 0, 0, 0, 1, 1},
			{0, 0, 0, 0, 1, 1, 1, 1},
			{1, 1, 1, 1, 1, 1, 0, 0}
	};

	int duty = 0;

	Divider sweep_divider;
	bool sweep_enable = false;
	bool sweep_negate = false;
	bool sweep_reload = false;
	u8 sweep_shift = 0;
	u16 sweep_period = 0;
	bool p2;

	bool muted = false;
};

class Triangle : public Channel
{
public:
	Triangle() : Channel()
	{
		sequencer.steps = 32;
		sequencer.sequence = seq;
	}

	void set_counter_reload_val( u8 val )
	{
		counter_reload_val = val;
	}

	void set_flag_linc_reload()
	{
		flag_linc_reload = true;
	}

	void tick_timer() override;

	void tick_lc();

	u8 get_dac_in() override;

	bool is_playing() override
	{
		return enabled && timer.get_period() >= 2 && (length > 0 && linear_counter > 0);
	}

private:
	static constexpr u8 seq[32] = {
			15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};

	u8 linear_counter = 0;
	u8 counter_reload_val = 0;

	bool flag_linc_reload = false;
};

class Noise : public Channel
{
public:
	Noise() : Channel()
	{
		sequencer.steps = 1;
	}

	void set_mode( bool m )
	{
		mode = m;
	}

	void set_period( u8 p )
	{
		timer.set_period( periods[p % 16] / 2 );
	}

	void tick_timer() override;

	u8 get_dac_in() override;

	bool is_playing() override
	{
		return enabled && length > 0;
	}

private:
	static constexpr u16 periods[16] = {
			4, 8, 16, 32, 64, 96, 128, 160, 202,
			254, 380, 508, 762, 1016, 2034, 4068
	};

	bool mode = false;
	u16 shifter = 1;
};

class DMC : public Channel
{
public:
	DMC() : Channel() {}

	void set_cpu( CPU *cpu )
	{
		this->cpu = cpu;
	}

	void tick_timer() override;

	void set_enabled( bool enable ) override
	{
		if ( !enable )
		{
			bytes_remaining = 0;
		}
		else if ( bytes_remaining == 0 )
		{
			start_sample();
		}
	}

	void set_output( u8 output )
	{
		this->output = GET_BITS(output, 0, 7);
	}

	void set_sample_address( u8 address )
	{
		this->sample_addr = 0xC000 | (((u16)address) << 6);
	}

	void set_sample_length( u8 length )
	{
		this->sample_length = 0x1 | (((u16)length) << 4);
	}

	void clear_interrupt()
	{
		irq_pending = false;
	}

	void set_status( u8 status )
	{
		irq_enabled = GET_BIT( status, 7 );
		if ( !irq_enabled )
		{
			irq_pending = false;
		}
		loop = GET_BIT( status, 6 );
		timer.set_period( periods[ GET_BITS( status, 0, 4 ) ] / 2 );
	}

	u16 get_bytes_remaining()
	{
		return bytes_remaining;
	}

	bool get_irq_pending()
	{
		return irq_pending;
	}

	u8 get_dac_in() override;

	bool is_playing() override
	{
		return true;
	}

private:
	static constexpr u16 periods[ 16 ] = {
			428, 380, 340, 320, 286, 254, 226, 214,
			190, 160, 142, 128, 106, 84, 72, 54
	};

	CPU *cpu;

	bool irq_enabled = false;
	bool irq_pending = false;
	bool loop = false;

	u8 sample_buffer = 0;
	bool sample_buffer_empty = true;
	u16 sample_addr = 0;
	u16 sample_length = 0;

	// Memory reader
	u16 addr_counter = 0;
	u16 bytes_remaining = 0;

	// Output unit
	u8 shifter = 0;
	u8 bits_remaining = 0;
	u8 output = 0;
	bool silence = true;

	void start_sample()
	{
		addr_counter = sample_addr;
		bytes_remaining = sample_length;
	}

	void read_sample_next()
	{
		if ( sample_buffer_empty && bytes_remaining > 0 )
		{
			sample_buffer = *cpu->get_mapper()->map_cpu(addr_counter);
			sample_buffer_empty = false;

			if ( ++addr_counter == 0x0 )
			{
				addr_counter = 0x8000;
			}

			if ( --bytes_remaining == 0 )
			{
				if ( loop )
				{
					start_sample();
				}
				else if ( irq_enabled )
				{
					irq_pending = true;
				}
			}

			cpu->skip_cycles( 4, READ ); // TODO make more accurate
		}
	}

	void tick_sample()
	{
		if ( bits_remaining == 0 )
		{
			bits_remaining = 8;
			silence = sample_buffer_empty;
			if ( !silence )
			{
				shifter = sample_buffer;
				sample_buffer_empty = true;
			}
		}

		if ( !silence )
		{
			bool bit_0 = GET_BIT( shifter, 0 );
			if ( bit_0 && output <= 125 )
			{
				output += 2;
			}
			else if ( !bit_0 && output >= 2 )
			{
				output -= 2;
			}
		}
		shifter >>= 1;
		bits_remaining -= 1;
	}
};