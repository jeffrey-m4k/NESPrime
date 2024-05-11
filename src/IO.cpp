#include "IO.h"

void IO::poll()
{
	auto *keystate = const_cast<Uint8 *>(SDL_GetKeyboardState( nullptr ));
	for ( int k = 0; k < 8; k++ )
	{
		SET_BIT( joy_status[ 0 ], k, keystate[ bindings[ k ] ] );
		SET_BIT( joy_status[ 1 ], k, 0 );

		// Handle impossible button combos (Zelda II left+right glitch)
		for ( int b = 4; b <= 6; b += 2 )
		{
			if ( GET_BIT( joy_status[ 0 ], b ) && GET_BIT( joy_status[ 0 ], b+1 ) )
			{
				CLEAR_BIT( joy_status[ 0 ], b );
				CLEAR_BIT( joy_status[ 0 ], b+1 );
			}
		}
	}
}

bool IO::read_joy()
{
	bool val = GET_BIT( joy_status[ 0 ], 0 );

	joy_status[0] >>= 1;
	SET_BIT( joy_status[ 0 ], 7 );

	return val;
}