#pragma once

#include "../AudioDevice.hpp"

#include <mutex>

#include <SDL2/SDL_audio.h>

namespace GBA::audio {
	using namespace common;

	class SdlAudioDevice : public AudioDevice {
	public :
		SdlAudioDevice();

		~SdlAudioDevice() override;

		void Start() override;
		void Stop() override;

		void PushSamples(common::i16* samples) override;

		friend void feed_callback(void* userdata, uint8_t* stream, int len);

	private :
		std::mutex m_buffer_mutex;
		i16* m_buffer;

		SDL_AudioSpec m_spec;
		SDL_AudioDeviceID m_dev_id;
	};
}