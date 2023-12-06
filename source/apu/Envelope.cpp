#include "../../apu/Envelope.hpp"

namespace GBA::apu {
	Envelope::Envelope() :
		m_ticks{1},
		m_curr_vol{}, m_shadow{},
		m_step{}, m_dir{}, m_init{}
	{}

	void Envelope::Restart() {
		m_shadow.dir = m_dir;
		m_shadow.init = m_init;
		m_shadow.step = m_step;
		m_curr_vol = m_init;
		m_ticks = 1;
	}

	u32 Envelope::GetCurrVolume() const {
		return m_curr_vol;
	}

	void Envelope::SetStepTime(u8 step) {
		m_step = step;
	}

	void Envelope::SetDirection(bool dir) {
		m_dir = dir;
	}

	void Envelope::SetInitVol(u8 init) {
		m_init = init;
	}

	bool Envelope::IsDacOn() const {
		return m_init || m_dir;
	}

	void Envelope::Update() {
		if (!m_step)
			return;

		m_ticks++;

		if (m_ticks >= m_shadow.step) {
			m_ticks = 1;

			if (m_shadow.dir) {
				if (m_curr_vol < 15)
					m_curr_vol++;
			}
			else {
				if (m_curr_vol > 0)
					m_curr_vol--;
			}
		}
	}
}