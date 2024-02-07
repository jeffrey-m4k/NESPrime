#pragma once

#include <map>
#include <cstdint>
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

	virtual void set_timer_hi( uint8_t hi );

	void set_length_counter( uint8_t c );

	void set_timer_lo( uint8_t lo )
	{
		timer.set_lo( lo );
	}

	void set_length_halt( bool halt );

	void set_constant_vol( bool cv )
	{
		envelope.set_constant_vol( cv );
	}

	void set_vol( uint8_t v )
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

	virtual float get_waveform_at_time( float time ) = 0;

	virtual bool is_playing() = 0;

	virtual uint8_t get_dac_in() = 0;

	float get_output();

public:
	bool debug_muted = false;

protected:
	bool enabled = false;

	Divider timer;
	Sequencer sequencer;
	Envelope envelope;
	uint8_t seq_out = 0;

	uint8_t dac_in_last = 0;
	float dac_out_last = 0.0;

	uint8_t length = 0;
	bool length_halt = false;

	static constexpr uint8_t length_lookup[2][16]{
			{
					0x0A, 0x14, 0x28, 0x50, 0xA0, 0x3C, 0x0E, 0x1A,
					0x0C, 0x18, 0x30, 0x60, 0xC0, 0x48, 0x10, 0x20
			},
			{
					0xFE, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
					0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E
			}
	};

	virtual float normalize_volume( int vol )
	{
		return 0.5 + ((vol - (envelope.get_volume() / 2.0)) / 15.0);
	}
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

	void set_timer_hi( uint8_t hi ) override;

	void set_duty( uint8_t duty );

	void set_p2( bool is_p2 )
	{
		p2 = is_p2;
	}

	uint8_t get_dac_in() override;

	void update_sweep( uint8_t byte );

	void tick_sweep();

	bool is_playing() override
	{
		return enabled && timer.get_period() > 8 && !muted && length > 0;
	}

	float get_waveform_at_time( float time ) override
	{
		if ( !is_playing() ) return 0.5;

		// TODO remove magic number (make global const for cycle rates - 1.79MHz is CPU frequency)
		float period = 1 / (1789773.0 / (16 * (timer.get_period() + 1)));
		double period_mod = fmod( time, period );
		if ( period_mod < 0 )
		{
			period_mod = period + period_mod;
		}
		int step = sequencer.steps * (period_mod / period);
		return normalize_volume( envelope.get_volume() * sequencer.sequence[ step ] );
	}

private:
	static constexpr uint8_t seqs[4][8] = {
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
	uint8_t sweep_shift = 0;
	uint16_t sweep_period = 0;
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

	void set_counter_reload_val( uint8_t val )
	{
		counter_reload_val = val;
	}

	void set_flag_linc_reload()
	{
		flag_linc_reload = true;
	}

	void tick_timer() override;

	void tick_lc();

	uint8_t get_dac_in() override;

	bool is_playing() override
	{
		return enabled && timer.get_period() >= 2 && (length > 0 && linear_counter > 0);
	}

	float get_waveform_at_time( float time ) override
	{
		if ( !is_playing() ) return 0.5;

		// TODO remove magic number (make global const for cycle rates - 1.79MHz is CPU frequency)
		float period = 1 / (1789773.0 / (32 * (timer.get_period() + 1)));
		double period_mod = fmod( time, period );
		if ( period_mod < 0 )
		{
			period_mod = period + period_mod;
		}
		int step = sequencer.steps * (period_mod / period);
		return normalize_volume( sequencer.sequence[ step ] );
	}

private:
	static constexpr uint8_t seq[32] = {
			15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};

	uint8_t linear_counter = 0;
	uint8_t counter_reload_val = 0;

	bool flag_linc_reload = false;

	float normalize_volume( int vol ) override
	{
		return 0.5 + ((vol - (15.0 / 2.0)) / 15.0);
	}
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

	void set_period( uint8_t p )
	{
		timer.set_period( periods[p % 16] );
	}

	void tick_timer() override;

	uint8_t get_dac_in() override;

	bool is_playing() override
	{
		return enabled && length > 0;
	}

	float get_waveform_at_time( float time ) override
	{
		if ( !is_playing() ) return 0.5;

		float period = 1 / (1789773.0 / (16 * (timer.get_period() + 1)));
		int step = floor( time / period );

		static std::default_random_engine generator( std::random_device{}() );
		generator.seed( step + waveform_rand );
		static std::bernoulli_distribution distribution( 0.5 );
		distribution.reset();

		return normalize_volume( envelope.get_volume() * distribution( generator ) );
	}

	uint16_t waveform_rand = 0;

private:
	static constexpr uint16_t periods[16] = {
			4, 8, 16, 32, 64, 96, 128, 160, 202,
			254, 380, 508, 762, 1016, 2034, 4068
	};

	bool mode;
	uint16_t shifter = 1;
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

	void set_output( uint8_t output )
	{
		this->output = output & 0x7F;
	}

	void set_sample_address( uint8_t address )
	{
		this->sample_addr = 0xC000 | (((uint16_t)address) << 6);
	}

	void set_sample_length( uint8_t length )
	{
		this->sample_length = 0x1 | (((uint16_t)length) << 4);
	}

	void clear_interrupt()
	{
		irq_pending = false;
	}

	void set_status( uint8_t status )
	{
		irq_enabled = (status >> 7) & 0x1;
		if ( !irq_enabled )
		{
			irq_pending = false;
		}
		loop = (status >> 6) & 0x1;
		timer.set_period( periods[status & 0xF] / 2 );
	}

	uint16_t get_bytes_remaining()
	{
		return bytes_remaining;
	}

	bool get_irq_pending()
	{
		return irq_pending;
	}

	uint8_t get_dac_in() override;

	bool is_playing() override
	{
		return true;
	}

	float get_waveform_at_time( float time ) override
	{
		if ( silence ) return 0.5;

		float period = 1 / (1789773.0 / (16 * (timer.get_period() + 1))) / 4.0;
		int step = floor( time / period );
		if ( step < 0 ) step = sample_length * 8 + step;

		int byte = step / 8 % sample_length;
		int bit = step % 8;

		uint8_t out;

		if ( waveform_cache.empty() )
		{
			uint64_t sum = 0;
			uint32_t len = 0;
			uint8_t last_out = 0;
			for ( int byte_num = 0; byte_num < sample_length; ++byte_num )
			{
				uint8_t sample_byte = *cpu->get_mapper()->map_cpu( sample_addr + byte_num );
				for ( int bit_num = 0; bit_num < 8; ++bit_num )
				{
					bool delta = (sample_byte >> bit_num) & 0x1;
					if ( delta && last_out <= 125 )
					{
						last_out += 2;
					}
					else if ( !delta && last_out >= 2 )
					{
						last_out -= 2;
					}

					sum += last_out;
					++len;

					waveform_cache.insert( std::make_pair( std::make_pair( byte_num, bit_num ), last_out ) );
				}
			}

			waveform_avg = (sum / (float)len) / 127.0;
		}
		
		byte += (sample_length - bytes_remaining);
		if ( byte >= sample_length )
		{
			byte -= sample_length;
		}

		auto it = waveform_cache.find( std::make_pair( byte, bit ) );
		if ( it != waveform_cache.end() )
		{
			return it->second / 127.0 + (0.5 - waveform_avg);
		}
		else
		{
			return 0.5;
		}
	}

	void clear_waveform_cache()
	{
		waveform_cache.clear();
		waveform_avg = 0.0;
	}

private:
	static constexpr uint16_t periods[ 16 ] = {
			428, 380, 340, 320, 286, 254, 226, 214,
			190, 160, 142, 128, 106, 84, 72, 54
	};

	std::map< std::pair< int, int >, uint8_t > waveform_cache;
	float waveform_avg = 0.0;

	CPU *cpu;

	bool irq_enabled = false;
	bool irq_pending = false;
	bool loop = false;

	uint8_t sample_buffer = 0;
	bool sample_buffer_empty = true;
	uint16_t sample_addr = 0;
	uint16_t sample_length = 0;

	// Memory reader
	uint16_t addr_counter = 0;
	uint16_t bytes_remaining = 0;

	// Output unit
	uint8_t shifter = 0;
	uint8_t bits_remaining = 0;
	uint8_t output = 0;
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
			bool bit_0 = shifter & 0x1;
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