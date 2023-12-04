#include "../../apu/Sequencer.hpp"

namespace GBA::apu {
	Sequencer::Sequencer(bool has_sweep) : 
		m_has_sweep{has_sweep}, m_last_timestamp{},
		m_sweep{ nullptr }, m_envelope{},
		m_enable_len_counter {} {
		if (m_has_sweep)
			m_sweep = new Sweep();
	}

	SeqEvent Sequencer::Update(std::uint64_t new_timestamp) {
		SeqEvent ev = NONE;

		if (m_has_sweep) {
			if (!m_sweep->Update(new_timestamp))
				ev = SWEEP_END;
		}

		m_envelope.Update(new_timestamp);

		m_last_timestamp = new_timestamp;

		return ev;
	}

	Sequencer::~Sequencer() {
		if (m_sweep)
			delete m_sweep;
	}

	void Sequencer::Restart(u32 freq) {
		if (m_sweep)
			m_sweep->Restart(freq);
		m_envelope.Restart();
	}

	u32 Sequencer::GetFreq() const {
		if (m_has_sweep)
			return m_sweep->GetFreq();
		return 0x0;
	}
}