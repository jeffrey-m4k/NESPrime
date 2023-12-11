#pragma once

#include <algorithm>
#include <cstdint>

class Memory
{
public:
	Memory() : size( 0 ), mem( nullptr )
	{
	};

	Memory( const uint32_t size ) : size( size )
	{
		mem = new uint8_t[size];
	};

	~Memory()
	{
		std::fill( mem, mem + size, 0 );
		delete[] mem;
	};

	void init( uint32_t size );

	uint32_t get_size() const
	{
		return size;
	};

	uint8_t *get_mem() const
	{
		return mem;
	};
private:
	uint32_t size;
	uint8_t *mem;
};