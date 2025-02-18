#pragma once

#include "../common/Defs.hpp"
#include "Sequencer.hpp"
#include "BaseChannel.hpp"

namespace GBA::apu {
	class NoiseChannel : public BaseChannel {
	public:
		NoiseChannel();

		void SetMMIO(memory::MMIO* mmio);
		void Restart();

		void RegisterEventTypes() override;

		static constexpr uint64_t CPU_CYCLES = 16'780'000;
		static constexpr uint64_t ENVELOPE_BASE_CYCLES = 262'187;
		static constexpr uint64_t LEN_CYCLES = 65'547;
		static constexpr uint64_t DIV_CYCLES = 32'773;
		static constexpr uint64_t BASE_HZ = 524288;

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_seq);
			ar(m_enabled);
			ar(m_lsfr);
			ar(m_envelope_control.raw);
			ar(m_control.raw);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_seq);
			ar(m_enabled);
			ar(m_lsfr);
			ar(m_envelope_control.raw);
			ar(m_control.raw);
		}

	private :
		Sequencer m_seq;
		bool m_enabled;

		u16 m_lsfr;

		union {
			struct {
				u8 len : 6;
				u8 : 2;
				u8 envelope_step : 3;
				bool envelope_dir : 1;
				u8 envelope_init : 4;
			};

			u16 raw;
		} m_envelope_control;

		union {
			struct {
				u8 freq_div : 3;
				bool counter_width : 1;
				u8 shift_freq : 4;
				u8 : 6;
				bool stop_on_len : 1;
			};

			u16 raw;
		} m_control;

		friend void noise_seq_update(void* userdata);
		friend void noise_sample_update(void* userdata);
	};
}