#include "SdlAudioDevice.hpp"

#include <bit>
#include <condition_variable>

#include "../../common/Error.hpp"

namespace GBA::audio {
	void feed_callback(void* userdata, uint8_t* stream, int len) {
		SdlAudioDevice* device = std::bit_cast<SdlAudioDevice*>(userdata);
		i16* dest = std::bit_cast<i16*>(stream);

		std::unique_lock _lk{ device->m_buffer_mutex };

		auto avail = device->m_buffer.readAvailable();

		len /= 2;

		auto remain = len - avail;

		device->m_buffer.readBuff(dest, len - 2);
		i16 last_left = 0;
		i16 last_right = 0;

		i16 dir_l = last_left > 0 ? -1 : 1;
		i16 dir_r = last_right > 0 ? -1 : 1;

		device->m_buffer.readBuff(&last_left, 1);
		device->m_buffer.readBuff(&last_right, 1);

		while (remain > 0) {
			dest[avail++] = last_left;
			dest[avail++] = last_right;

			if (last_left)
				last_left += dir_l;

			if (last_right)
				last_right += dir_r;

			remain -= 2;
		}

		_lk.unlock();
		device->m_cv.notify_one();
	}

	SdlAudioDevice::SdlAudioDevice() : 
		m_buffer_mutex{}, m_buffer{},
		m_spec{}, m_ready(false),
		m_cv{} {
		SDL_AudioSpec wanted_audio{};

		wanted_audio.channels = 2;
		wanted_audio.userdata = std::bit_cast<void*>(this);
		wanted_audio.callback = feed_callback;
		wanted_audio.format = AUDIO_S16;
		wanted_audio.freq = 32768;
		wanted_audio.samples = 2048;

	    m_dev_id = SDL_OpenAudioDevice(
			nullptr, 0, &wanted_audio, &m_spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE
		);

		if (!m_dev_id)
			throw std::runtime_error("Could not open audio device");
	}

	SdlAudioDevice::~SdlAudioDevice() {}

	void SdlAudioDevice::Start() {
		SDL_PauseAudioDevice(m_dev_id, 0);
	}

	void SdlAudioDevice::Stop() {
		SDL_PauseAudioDevice(m_dev_id, 1);
		SDL_CloseAudioDevice(m_dev_id);
	}

	void SdlAudioDevice::PushSamples(common::i16* samples) {
		std::unique_lock _lk{ m_buffer_mutex };
		m_buffer.writeBuff(samples, 1024);
		_lk.unlock();
	}

	void SdlAudioDevice::PushSample(common::i16 sample_l, common::i16 sample_r) {
		std::unique_lock _lk{ m_buffer_mutex };

		m_cv.wait(_lk, [this]() { return m_buffer.writeAvailable() >= 2; });

		/*if (m_buffer.writeAvailable() < 2) {
			error::DebugBreak();
		}*/

		m_buffer.insert(&sample_l);
		m_buffer.insert(&sample_r);
		_lk.unlock();
	}
}