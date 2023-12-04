#pragma once

#include "../common/Defs.hpp"

#include "Sequencer.hpp"

#include <functional>

namespace GBA::memory {
	class MMIO;
	class EventScheduler;
}

namespace GBA::apu {
	using namespace common;

	using EnableCallback = std::function<void(bool)>;

	class SquareChannel {
	public :
		SquareChannel(bool has_sweep);

		void SetMMIO(memory::MMIO* mmio);
		void SetScheduler(memory::EventScheduler* sched);
		void SetEnableCallback(EnableCallback callback);

		i16 GetSample() const;
		i16 GetNumAccum() const;
		void ResetAccum();

		static constexpr uint64_t SWEEP_CYCLES = 131'093;
		static constexpr uint64_t CPU_CYCLES = 16'780'000;
		static constexpr uint64_t ENVELOPE_BASE_CYCLES = 262'187;
		static constexpr uint64_t LEN_CYCLES = 65'547;

		static constexpr u8 WAVEFORMS[4] = {
			0b00000001,
			0b00000011,
			0b00001111,
			0b00111111
		};

	private :
		union {
			struct {
				u8 len : 6;
				u8 pattern : 2;
				u8 time : 3;
				bool direction : 1;
				u8 init_vol : 4;
			};

			u16 raw;
		} m_envelope_control;

		union {
			struct {
				u8 : 8;
				u8 : 3;
				u8 : 3;
				bool stop_on_len : 1;
			};

			u16 raw;
		} m_control;

		bool m_has_sweep;
		memory::EventScheduler* m_sched;

		u8 m_curr_sample;
		u8 m_curr_wave_pos;
		u32 m_curr_freq;

		bool m_enabled;

		EnableCallback m_en_callback;

		u8 m_sample_accum;

		Sequencer m_seq;

		friend void seq_update(void* userdata);
		friend void sample_update(void* userdata);

		void Restart();
	};
}