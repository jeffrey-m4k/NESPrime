#include "APU.h"
#include "../CPU.h"
#include "../Display.h"

APU::APU()
{
	SDL_zero( audio_spec );
	audio_spec.freq = SAMPLE_RATE * 1;
	audio_spec.format = AUDIO_F32SYS;
	audio_spec.channels = 1;
	audio_spec.samples = 1024;
	audio_spec.callback = nullptr;

	audio_device = SDL_OpenAudioDevice( nullptr, 0, &audio_spec, nullptr, 0 );

	SDL_PauseAudioDevice( audio_device, 0 );

	sc_2a03.frameSeq.set_chip( &sc_2a03 );
	sc_2a03.pulse[1].set_p2( true );
}

APU::~APU()
{
	SDL_CloseAudioDevice( audio_device );
}

void APU::init()
{
	sc_2a03.dmc.set_cpu( get_nes()->get_cpu() );
	Mapper *mapper = nes->get_cpu()->get_mapper();
	mapper->set_sound_chip( get_chip( mapper->get_sound_chip_type() ) );
	nes->get_display()->init_apu_display();
}

void APU::cycle()
{
	sc_2a03.clock();
	sc_5b.clock();
	sample();

	if ( sc_2a03.frameSeq.interrupt )
	{
		get_nes()->get_cpu()->trigger_irq();
	}
}

void APU::sample()
{
	sample_buffer_raw.push_back( get_mixer() * 50 );

	if ( ++sample_clock >= sample_per * nes->get_emu_speed() )
	{
		low_pass();
		downsample();
		sample_clock -= sample_per * nes->get_emu_speed();

		if ( nes->DEBUG_APU )
		{
			std::vector<float> debug_waveforms;

			for ( SoundChip *ec : nes->get_display()->get_apu_chips() )
			{
				for ( int c = 0; c < ec->get_channel_count(); ++c )
				{
					debug_waveforms.push_back( ec->peek_output( c ) / ec->get_debug_damping( c ) );
				}
			}

			debug_waveforms.push_back( sample_buffer_raw[0] );

			nes->get_display()->push_apu_samples(debug_waveforms);
		}
		sample_buffer_raw.clear();
	}
	
	if ( sample_buffer.size() >= 100 )
	{
		SDL_QueueAudio( audio_device, sample_buffer.data(), sample_buffer.size() * 4 );
		sample_buffer.clear();
	}
}

void APU::low_pass()
{
	float cutoff_freq = 18000.0;

	float omega_c = 2 * M_PI * cutoff_freq / (sample_per * SAMPLE_RATE * nes->get_emu_speed());
	float alpha = 1 / (1 + omega_c);

	for ( int i = 0; i < sample_buffer_raw.size(); ++i )
	{
		float filtered = alpha * (sample_buffer_raw[i] + low_pass_last);
		low_pass_last = filtered;
		sample_buffer_raw[ i ] = filtered * (1 - alpha);
	}
}

void APU::downsample()
{
	sample_buffer.push_back( sample_buffer_raw[0] * 5 );
}

float APU::get_mixer()
{
	float sample =
		sc_2a03.get_output() +
		sc_5b.get_output();

	return sample;
}

void APU::write_apu_reg( u8 reg, u8 data )
{
	sc_2a03.write_reg( reg, data );
}

u8 APU::read_status()
{
	// TODO fix this
	u8 s =
		sc_2a03.pulse[0].get_length_halt()				|
		sc_2a03.pulse[1].get_length_halt() << 1			|
		sc_2a03.triangle.get_length_halt() << 2			|
		sc_2a03.noise.get_length_halt() << 3			|
		(sc_2a03.dmc.get_bytes_remaining() > 0) << 4	|
		sc_2a03.frameSeq.interrupt << 6					|
		sc_2a03.dmc.get_irq_pending() << 7;

	sc_2a03.frameSeq.interrupt = false;

	return s;
}

SoundChip *APU::get_chip( SCType type )
{
	switch ( type )
	{
		case SCType::RICOH_2A03:
			return &sc_2a03;
		case SCType::SUNSOFT_5B:
			return &sc_5b;
		default:
			return nullptr;
	}
}