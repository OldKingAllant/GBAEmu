#include "../../apu/Sequencer.hpp"

namespace GBA::apu {
	Sequencer::Sequencer(bool has_sweep) : 
		m_has_sweep{has_sweep}, m_last_timestamp{},
		m_sweep{ nullptr }, m_envelope{}, m_len_counter{},
		m_enable_len_counter{}, m_curr_update_count{} {
		if (m_has_sweep)
			m_sweep = new Sweep();
	}

	SeqEvent Sequencer::Update(std::uint64_t new_timestamp) {
		SeqEvent ev = NONE;

		if ((m_curr_update_count & 1) == 0) {
			if (m_enable_len_counter) { //Always updated at 256Hz
				if (!m_len_counter.Update())
					ev = LEN_EXPIRED;
			}
		}

		if ((m_curr_update_count & 3) == 2) {
			if (m_has_sweep) { //Should be updated at 128Hz
				if (!m_sweep->Update())
					ev = SWEEP_END;
			}
		}

		if (m_curr_update_count == 7) {
			m_envelope.Update(); //Should be updated at 64Hz
		}

		m_curr_update_count++; //Incremented at 256Hz

		if (m_curr_update_count >= 8)
			m_curr_update_count = 0;

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

		m_curr_update_count = 0;
	}

	u32 Sequencer::GetFreq() const {
		if (m_has_sweep)
			return m_sweep->GetFreq();
		return 0x0;
	}

	u8 Sequencer::GetVolume() const {
		return (u8)m_envelope.GetCurrVolume();
	}
}