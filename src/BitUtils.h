#pragma once

#include <stdint.h>
#include <cmath>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

template <typename T>
inline const bool GET_BIT( T integer, int bit )
{
	return (integer >> bit) & 0x1;
}

template <typename T>
inline const T GET_BITS( T integer, int start_bit, int bits )
{
	T mask = (1 << bits) - 1;
	return (integer >> start_bit) & mask;
}

template <typename T>
inline void SET_BIT( T &integer, int bit )
{
	integer |= (1 << bit);
}

template <typename T>
inline void CLEAR_BIT( T &integer, int bit )
{
	integer &= ~(1 << bit);
}

template <typename T>
inline void SET_BIT( T &integer, int bit, bool val )
{
	if ( val )
	{
		SET_BIT( integer, bit );
	}
	else
	{
		CLEAR_BIT( integer, bit );
	}
}
