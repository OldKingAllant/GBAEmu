#include "../../apu/Sweep.hpp"

namespace GBA::apu {
	Sweep::Sweep() : m_control{},
		m_ticks{},
		m_freq{}
	{}

	u32 Sweep::Update() {
		if (!m_control.time)
			return m_freq;

		m_ticks++;

		if (m_ticks >= m_control.time) {
			m_ticks -= m_control.time;

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