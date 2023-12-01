#pragma once

#include "../AudioDevice.hpp"

#include <mutex>

#include <SDL2/SDL_audio.h>

#include "../../thirdparty/ringbuffer/ringbuffer.hpp"

namespace GBA::audio {
	using namespace common;

	class SdlAudioDevice : public AudioDevice {
	public :
		SdlAudioDevice();

		~SdlAudioDevice() override;

		void Start() override;
		void Stop() override;

		void PushSamples(common::i16* samples) override;
		void PushSample(common::i16 sample_l, common::i16 sample_r) override;

		friend void feed_callback(void* userdata, uint8_t* stream, int len);

	private :
		std::mutex m_buffer_mutex;
		jnk0le::Ringbuffer<i16, 4096> m_buffer;

		bool m_ready;
		std::condition_variable m_cv;

		SDL_AudioSpec m_spec;
		SDL_AudioDeviceID m_dev_id;
	};
}