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
		static constexpr uint64_t DIV_CYCLES = 32'773;
		static constexpr uint64_t BASE_HZ = 131072;
		static constexpr u32 SAMPLE_RATE = (u32)(SquareChannel::CPU_CYCLES / SquareChannel::BASE_HZ);

		static constexpr i16 WAVEFORMS[4][8] = {
			{ 1, 0, 0, 0, 0, 0, 0, 0 },
			{ 1, 1, 0, 0, 0, 0, 0, 0 },
			{ 1, 1, 1, 1, 0, 0, 0, 0 },
			{ 1, 1, 1, 1, 1, 1, 0, 0 }
		};

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_envelope_control.raw);
			ar(m_control.raw);
			ar(m_curr_wave_pos);
			ar(m_curr_freq);
			ar(m_enabled);
			ar(m_seq);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_envelope_control.raw);
			ar(m_control.raw);
			ar(m_curr_wave_pos);
			ar(m_curr_freq);
			ar(m_enabled);
			ar(m_seq);
		}

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

		i16 m_curr_sample;
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