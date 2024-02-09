#pragma once

#include <algorithm>
#include <cstdint>
#include "BitUtils.h"

class Memory
{
public:
	Memory() : size( 0 ), mem( nullptr )
	{
	};

	Memory( const u32 size ) : size( size )
	{
		mem = new u8[size];
	};

	~Memory()
	{
		std::fill( mem, mem + size, 0 );
		delete[] mem;
	};

	void init( u32 size );

	u32 get_size() const
	{
		return size;
	};

	u8 *get_mem() const
	{
		return mem;
	};
private:
	u32 size;
	u8 *mem;
};