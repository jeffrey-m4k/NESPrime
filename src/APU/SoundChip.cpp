#include "SoundChip.h"

SoundChip::SoundChip()
{
	const std::string notes[] = {
		"C", "C#", "D", "D#", "E", "F",
		"F#", "G", "G#", "A", "A#", "B"
	};
	int note = 9;
	int octave = 0;

	for ( int n = -48; n <= 47; ++n )
	{
		const double a4_freq = 440.0;
		double freq = a4_freq * std::pow( std::pow( 2.0, 1.0 / 12.0 ), n );
		note_freqs[ freq ] = notes[ note ] + std::to_string( octave );

		if ( ++note >= 12 )
		{
			note = 0;
			octave++;
		}
	}
}

std::string SoundChip::freq_to_note( double freq )
{
	auto it = note_freqs.lower_bound( freq );
	if ( it != note_freqs.begin() )
	{
		auto prev_it = std::prev( it );
		if ( it == note_freqs.end() || std::abs( prev_it->first - freq ) < std::abs( it->first - freq ) )
		{
			it = prev_it;
		}
	}

	return it->second;
}