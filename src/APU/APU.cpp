#include "APU.h"
#include "../CPU.h"
#include "../Display.h"

APU::APU()
{
	SDL_setenv( "SDL_AUDIODRIVER", "directsound", 1 );
	SDL_zero( audio_spec );
	audio_spec.freq = SAMPLE_RATE * 1;
	audio_spec.format = AUDIO_F32SYS;
	audio_spec.channels = 1;
	audio_spec.samples = 1024;
	audio_spec.callback = nullptr;

	audio_device = SDL_OpenAudioDevice( nullptr, 0, &audio_spec, nullptr, 0 );

	SDL_PauseAudioDevice( audio_device, 0 );

	frameSeq.set_apu( this );

	pulse[1].set_p2( true );
}

APU::~APU()
{
	SDL_CloseAudioDevice( audio_device );
}

void APU::init()
{
	dmc.set_cpu( get_nes()->get_cpu() );
	Mapper *mapper = nes->get_cpu()->get_mapper();
	mapper->set_sound_chip( get_chip( mapper->get_sound_chip_type() ) );
}

void APU::cycle()
{
	if ( tick_fs )
	{
		frameSeq.tick();
		pulse[0].tick_timer();
		pulse[1].tick_timer();
		noise.tick_timer();
		dmc.tick_timer();
		tick_fs = false;
	}
	else
	{
		tick_fs = true;
	}

	ec_5b.clock();
	triangle.tick_timer();
	sample();

	if ( frameSeq.interrupt )
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
			nes->get_display()->push_apu_samples( std::vector<float>
			{
				pulse[ 0 ].peek_output(),
				pulse[ 1 ].peek_output(),
				triangle.peek_output(),
				noise.peek_output(),
				dmc.peek_output(),
				ec_5b.peek_output( 0 ),
				ec_5b.peek_output( 1 ),
				ec_5b.peek_output( 2 )
			} );
		}
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
		sample_buffer_raw[ i ] = filtered;
	}
}

void APU::downsample()
{
	sample_buffer.push_back( sample_buffer_raw[0]);
	sample_buffer_raw.clear();
}

float APU::get_mixer()
{
	float sample =
		0.00752 * (pulse[ 0 ].get_output() + pulse[ 1 ].get_output()) +
		0.00851 * triangle.get_output() +
		0.00494 * noise.get_output() +
		0.00335 * dmc.get_output() +
		0.0045 * ec_5b.get_output();

	return sample;
}

void APU::write_apu_reg( u8 reg, u8 data )
{
	int p2;
	switch ( reg )
	{
		case 0x0:
		case 0x4:
			p2 = reg == 0x4 ? 1 : 0;
			pulse[ p2 ].set_duty( GET_BITS( data, 6, 2 ) );
			pulse[ p2 ].set_length_halt( GET_BIT( data, 5 ) );
			pulse[ p2 ].set_constant_vol( GET_BIT( data, 4 ) );
			pulse[ p2 ].set_vol( GET_BITS( data, 0, 4 ) );
			break;
		case 0x1:
		case 0x5:
			p2 = reg == 0x5 ? 1 : 0;
			pulse[ p2 ].update_sweep( data );
			break;
		case 0x2:
		case 0x6:
			p2 = reg == 0x6 ? 1 : 0;
			pulse[ p2 ].set_timer_lo( data );
			break;
		case 0x3:
		case 0x7:
			p2 = reg == 0x7 ? 1 : 0;
			pulse[ p2 ].set_timer_hi( data );
			break;
		case 0x8:
			triangle.set_length_halt( GET_BIT(data, 7) );
			triangle.set_counter_reload_val( GET_BITS(data, 0, 7) );
			break;
		case 0xA:
			triangle.set_timer_lo( data );
			break;
		case 0xB:
			triangle.set_timer_hi( data );
			triangle.set_flag_linc_reload();
			break;
		case 0xC:
			noise.set_length_halt( GET_BIT( data, 5 ) );
			noise.set_constant_vol( GET_BIT( data, 4 ) );
			noise.set_vol( GET_BITS( data, 0, 4 ) );
			break;
		case 0xE:
			noise.set_mode( GET_BIT( data, 7 ) );
			noise.set_period( GET_BITS( data, 0, 4 ) );
			break;
		case 0xF:
			noise.set_length_counter( GET_BITS( data, 3, 5 ) );
			break;
		case 0x10:
			dmc.set_status( data );
			break;
		case 0x11:
			dmc.set_output( data );
			break;
		case 0x12:
			dmc.set_sample_address( data );
			break;
		case 0x13:
			dmc.set_sample_length( data );
			break;
		case 0x15:
			pulse[ 0 ].set_enabled( GET_BIT( data, 0 ) );
			pulse[ 1 ].set_enabled( GET_BIT( data, 1 ) );
			triangle.set_enabled( GET_BIT( data, 2 ) );
			noise.set_enabled( GET_BIT( data, 3 ) );
			dmc.set_enabled( GET_BIT( data, 4 ) );
			dmc.clear_interrupt();
			break;
		case 0x17:
			frameSeq.reset( data );
			break;
		default:
			break;
	}
}

u8 APU::read_status()
{
	// TODO fix this
	u8 s =
		pulse[0].get_length_halt()				|
		pulse[1].get_length_halt() << 1			|
		triangle.get_length_halt() << 2			|
		noise.get_length_halt() << 3			|
		(dmc.get_bytes_remaining() > 0) << 4	|
		frameSeq.interrupt << 6					|
		dmc.get_irq_pending() << 7;

	frameSeq.interrupt = false;

	return s;
}

Channel *APU::get_channel( int channel )
{
	if ( channel > 7 || channel < 0 )
	{
		channel = 0;
	}
	switch ( channel )
	{
		case 0: 
			return &pulse[ 0 ];
		case 1: 
			return &pulse[ 1 ];
		case 2: 
			return &triangle;
		case 3: 
			return &noise;
		case 4:
			return &dmc;
		case 5:
			return ec_5b.get_channel( 0 );
		case 6:
			return ec_5b.get_channel( 1 );
		case 7:
			return ec_5b.get_channel( 2 );
	}
}

void APU::set_debug_mute( bool mute, int channel )
{
	get_channel( channel )->debug_muted = mute;

	bool *disp_arr = nes->get_display()->apu_debug_muted;
	disp_arr[ channel ] = mute;
}

void APU::toggle_debug_mute( int channel )
{
	set_debug_mute( !get_channel( channel )->debug_muted, channel );
}

bool APU::is_playing( int channel )
{
	return get_channel( channel )->is_playing();
}

ExpansionChip *APU::get_chip( ECType type )
{
	switch ( type )
	{
		case ECType::SUNSOFT_5B:
			return &ec_5b;
		default:
			return nullptr;
	}
}