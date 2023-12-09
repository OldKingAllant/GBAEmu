#pragma once

#include "../common/Defs.hpp"

namespace GBA::audio {
	class AudioDevice {
	public :
		AudioDevice();

		virtual ~AudioDevice();

		virtual void Start() = 0;
		virtual void Stop() = 0;

		virtual void PushSamples(common::i16* samples) = 0;
		virtual void PushSample(common::i16 sample_l, common::i16 sample_r) = 0;

		virtual void AudioSync(bool sync) {
			m_sync = sync;
		}

	protected:
		bool m_sync;
	};
}