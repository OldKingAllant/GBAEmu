#include "../../apu/Sweep.hpp"

namespace GBA::apu {
	Sweep::Sweep() : m_control{},
		m_last_update_timestamp{},
		m_freq{}
	{}

	u32 Sweep::Update(uint64_t timestamp) {
		if (!m_control.time)
			return m_freq;

		uint64_t elapsed = (timestamp - m_last_update_timestamp)
			/ SWEEP_CYCLES;

		if (elapsed >= m_control.time) {
			m_last_update_timestamp = timestamp;

			int new_period = m_freq;

			if (m_control.direction) {
				new_period -= new_period / (2 << m_control.shift);
			}
			else {
				new_period += new_period / (2 << m_control.shift);
			}

			if (new_period < 0 || new_period > 0x7FF)
				new_period = 0;

			m_freq = (u32)new_period;
		}

		return m_freq;
	}

	void Sweep::Restart(u32 freq) {
		m_freq = freq;
	}

	u32 Sweep::GetFreq() const {
		return m_freq;
	}
}