#include "Memory.h"

void Memory::init( const u32 size )
{
	delete[] this->mem;
	this->size = size;
	this->mem = new u8[size];
}


