#pragma once

#include <iostream>
#include <cstdint>
#include <set>
#include <unordered_map>
#include "Memory.h"
#include "Component.h"

class Mapper;

class Processor : public Component
{
public:
	Processor();

	virtual void reset() = 0;

	virtual void init() = 0;

	virtual bool run();

	long get_cycle()
	{
		return cycle;
	}

	Memory *get_mem()
	{
		return &mem;
	}

	Mapper *get_mapper()
	{
		return mapper;
	}

	void set_mapper( Mapper *mapperIn )
	{
		mapper = mapperIn;
	}

protected:
	virtual u8 read( int addr ) = 0;

	virtual bool write( u16 addr, u8 data ) = 0;

protected:
	int idle_cycles = 0;
	long cycle = 0;
	Memory mem;
	Mapper *mapper;
};
