#pragma once

#include "../BitUtils.h"

class SC_2A03;

class Divider
{
public:
	explicit Divider( u16 p ) : period( p )
	{
	};

	bool clock();

	void reload();

	void set_period( int p )
	{
		period = p;
	}

	u16 get_period() const
	{
		return period;
	}

	u16 get_counter() const
	{
		return counter;
	}

	void set_hi( u8 hi )
	{
		period = (period & 0x00FF) | ((hi & 0x7) << 8);
	}//= (period & 0xF) | ((hi & 0x7) << 8); }
	void set_lo( u8 lo )
	{
		period = (period & 0x7F00) | lo;
	}

private:
	u16 period = 0;
	u16 counter = 0;
};

class Sequencer
{
public:
	explicit Sequencer( u8 s ) : steps( s ), step( 0 ), sequence( nullptr )
	{
	};

	u8 next();

	void reset()
	{
		step = 0;
	}

	u8 steps;
	u8 step;
	const u8 *sequence;
};

class Envelope
{
public:
	Envelope() : div( 15 )
	{
	};

	void clock();

	u8 get_volume() const
	{
		return constant ? param : decay;
	}

	void set_param( u8 p )
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
	u8 decay = 0;
	u8 param = 15;
};

class FrameSequencer
{
public:
	FrameSequencer() : divider( 3728 ), sequencer( 4 )
	{
	};

	void reset( u8 byte );

	void do_seq( u8 seq );

	void tick()
	{
		if ( divider.clock() )
		{
			do_seq( sequencer.next() );
		}
	};

	void set_chip( SC_2A03 *sc_2a03 )
	{
		sc = sc_2a03;
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

	SC_2A03 *sc;
};
