#include "SdlAudioDevice.hpp"

#include <bit>

namespace GBA::audio {
	void feed_callback(void* userdata, uint8_t* stream, int len) {
		SdlAudioDevice* device = std::bit_cast<SdlAudioDevice*>(userdata);

		std::scoped_lock<std::mutex> _lk{ device->m_buffer_mutex };

		std::fill_n(stream, len, 0);
		//std::copy_n(device->m_buffer, len, stream);
	}

	SdlAudioDevice::SdlAudioDevice() : 
		m_buffer_mutex{}, m_buffer{nullptr},
		m_spec{} {
		m_buffer = new i16[4096];

		SDL_AudioSpec wanted_audio{};

		wanted_audio.channels = 2;
		wanted_audio.userdata = std::bit_cast<void*>(this);
		wanted_audio.callback = feed_callback;
		wanted_audio.format = AUDIO_S16;
		wanted_audio.freq = 44100;
		wanted_audio.samples = 2048;

	    m_dev_id = SDL_OpenAudioDevice(
			nullptr, 0, &wanted_audio, &m_spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE
		);

		if (!m_dev_id)
			throw std::runtime_error("Could not open audio device");
	}

	SdlAudioDevice::~SdlAudioDevice() {
		delete[] m_buffer;
	}

	void SdlAudioDevice::Start() {
		SDL_PauseAudioDevice(m_dev_id, 0);
	}

	void SdlAudioDevice::Stop() {
		SDL_PauseAudioDevice(m_dev_id, 1);
		SDL_CloseAudioDevice(m_dev_id);
	}

	void SdlAudioDevice::PushSamples(common::i16* samples) {
		std::scoped_lock<std::mutex> _lk{ m_buffer_mutex };

		std::copy_n(samples, 4096, m_buffer);
	}
}