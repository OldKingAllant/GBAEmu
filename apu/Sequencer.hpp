#pragma once

#include <cstdint>

#include "Sweep.hpp"
#include "Envelope.hpp"

namespace GBA::apu {
	enum SeqEvent {
		NONE = 0,
		SWEEP_END = 1,
		LEN_EXPIRED = 2,
	};

	class Sequencer {
	public:
		Sequencer(bool has_sweep);

		SeqEvent Update(std::uint64_t new_timestamp);

		~Sequencer();

		void Restart(u32 freq);
		u32 GetFreq() const;

	private:
		bool m_has_sweep;
		std::uint64_t m_last_timestamp;

		Sweep* m_sweep;
		Envelope m_envelope;

		bool m_enable_len_counter;

		friend class SquareChannel;
	};
}