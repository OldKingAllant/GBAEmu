#pragma once

#include "BaseChannel.hpp"

namespace GBA::apu {
	class WaveChannel : public BaseChannel {
	public:
		WaveChannel();

		void SetMMIO(memory::MMIO* mmio);

		void Restart();

		static constexpr uint64_t DIV_CYCLES = 32'773;
		static constexpr uint64_t CPU_CYCLES = 16'780'000;
		static constexpr uint64_t BASE_HZ = 2097152;

	private:
		union {
			struct {
				u8 : 5;
				bool ram_dimension : 1;
				bool bank_number : 1;
				bool dac_enable : 1;
			};

			u16 raw;
		} m_control_l;

		union {
			struct {
				u8 len : 8;
				u8 : 5;
				u8 volume : 2;
				bool force_75 : 1;
			};

			u16 raw;
		} m_control_h;

		union {
			struct {
				u8 : 8;
				u8 : 6;
				bool stop_on_len : 1;
			};

			u16 raw;
		} m_freq_control;

		u32 m_freq;
		bool m_enable;

		u8 m_wave_ram[2][16];

		u8 m_curr_len;
		u8 m_curr_div_value;

		u8 m_curr_sample_pos;
		u8 m_curr_bank;

		friend void wave_seq_update(void* userdata);
		friend void wave_sample_update(void* userdata);
	};
}