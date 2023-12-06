#pragma once

#include <cstdint>

#include "Sweep.hpp"
#include "Envelope.hpp"
#include "LenCounter.hpp"

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
		u8 GetVolume() const;

	private:
		bool m_has_sweep;
		std::uint64_t m_last_timestamp;

		Sweep* m_sweep;
		Envelope m_envelope;
		LenCounter m_len_counter;

		bool m_enable_len_counter;

		uint64_t m_curr_update_count;

		friend class NoiseChannel;
		friend class SquareChannel;
	};
}