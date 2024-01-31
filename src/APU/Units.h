#pragma once

#include <cstdint>

class APU;

class Divider
{
public:
	explicit Divider( uint16_t p ) : period( p )
	{
	};

	bool clock();

	void reload();

	void set_period( int p )
	{
		period = p;
	}

	uint16_t get_period() const
	{
		return period;
	}

	uint16_t get_counter() const
	{
		return counter;
	}

	void set_hi( uint8_t hi )
	{
		period = (period & 0x00FF) | ((hi & 0x7) << 8);
	}//= (period & 0xF) | ((hi & 0x7) << 8); }
	void set_lo( uint8_t lo )
	{
		period = (period & 0x7F00) | lo;
	}

private:
	uint16_t period = 0;
	uint16_t counter = 0;
};

class Sequencer
{
public:
	explicit Sequencer( uint8_t s ) : steps( s ), step( 0 ), sequence( nullptr )
	{
	};

	uint8_t next();

	void reset()
	{
		step = 0;
	}

	uint8_t steps;
	uint8_t step;
	const uint8_t *sequence;
};

class Envelope
{
public:
	Envelope() : div( 15 )
	{
	};

	void clock();

	uint8_t get_volume() const
	{
		return constant ? param : decay;
	}

	void set_param( uint8_t p )
	{
		param = p;
	}

	void set_loop( bool l )
	{
		constant = l;
	}

	void set_constant_vol( bool c )
	{
		constant = c;
	}

	void restart()
	{
		start = true;
	}

private:
	Divider div;
	bool start = true;
	bool loop = false;
	bool constant = false;
	uint8_t decay = 0;
	uint8_t param = 15;
};

class FrameSequencer
{
public:
	FrameSequencer() : divider( 3728 ), sequencer( 4 )
	{
	};

	void reset( uint8_t byte );

	void do_seq( uint8_t seq );

	void tick()
	{
		if ( divider.clock() )
		{
			do_seq( sequencer.next() );
		}
	};

	void set_apu( APU *a )
	{
		apu = a;
	};
private:
	void set_interrupt();

	void do_length_clock();

	void do_env_clock();

public:
	bool interrupt = false;

private:
	Divider divider;
	Sequencer sequencer;

	bool irq_disable = false;

	APU *apu;
};
