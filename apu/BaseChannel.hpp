#pragma once

#include "../common/Defs.hpp"

#include <functional>

namespace GBA {
	namespace memory {
		class EventScheduler;
		class MMIO;
	}
}

namespace GBA::apu {
	using namespace common;

	using EnableCallback = std::function<void(bool)>;

	class BaseChannel {
	public:
		BaseChannel() : m_en_callback{}, m_sched{ nullptr },
			m_curr_sample{}, m_sample_accum{ 1 } {};

		void SetEnableCallback(EnableCallback callback) {
			m_en_callback = callback;
		}

		void SetScheduler(memory::EventScheduler* sched) {
			m_sched = sched;
		}

		i16 GetSample() const { return m_curr_sample; }
		i16 GetNumAccum() const { return m_sample_accum; }
		void ResetAccum() { m_sample_accum = 1; }

	protected:
		EnableCallback m_en_callback;
		memory::EventScheduler* m_sched;
		i16 m_curr_sample;
		u8 m_sample_accum;
	};
}