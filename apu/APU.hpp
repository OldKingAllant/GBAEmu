#pragma once

#include "../common/Defs.hpp"

#include <functional>

namespace GBA {
	namespace memory {
		class DMA;
		class MMIO;
	}
}

namespace GBA::apu {
	using namespace common;

	using SampleCallback = std::function<void(i16*)>;

	enum class ChannelId {
		FIFO_A,
		FIFO_B
	};

	enum ChannelStep {
		INC_LEFT = 1,
		INC_RIGHT = 2
	};

	class APU {
	public :
		APU();

		void SetDma(memory::DMA* _1, memory::DMA* _2);
		void SetCallback(SampleCallback callback, u32 required_samples);

		void Clock(u32 num_cycles);

		void SetMMIO(memory::MMIO* mmio);

		void TimerOverflow(u8 id);

		~APU();

	private :
		i16* m_sample_buffer;

		memory::DMA* m_dma1;
		memory::DMA* m_dma2;

		SampleCallback m_buffer_callback;
		u32 m_curr_samples;
		u32 m_req_samples;

		i8 m_internal_A_buffer[32];
		i8 m_internal_B_buffer[32];
		i8 m_A_pos;
		i8 m_B_pos;
		
		union {
			struct {
				u8 dmg_sound_volume : 2;
				u8 fifo_a_volume : 1;
				u8 fifo_b_volume : 1;
				u8 : 4;
				u8 : 0;
				u8 fifo_a_right : 1;
				u8 fifo_a_left : 1;
				u8 fifo_a_timer_sel : 1;
				u8 fifo_a_reset : 1;

				u8 fifo_b_right : 1;
				u8 fifo_b_left : 1;
				u8 fifo_b_timer_sel : 1;
				u8 fifo_b_reset : 1;
			};

			u16 raw;
		} m_soundcnt_h;

		union {
			struct {
				u8 sound_1_on : 1;
				u8 sound_2_on : 1;
				u8 sound_3_on : 1;
				u8 sound_4_on : 1;
				u8 : 3;
				u8 master_en : 1;
			};

			u8 raw;
		} m_soundcnt_x;

		union {
			struct {
				u8 : 1;
				u8 bias_low : 7;
				u8 : 0;
				u8 bias_high : 2;
				u8 : 4;
				u8 amplitude : 2;
			};

			u16 raw;
		} m_soundbias;

		u32 m_interleaved_buffer_pos_l;
		u32 m_interleaved_buffer_pos_r;

		u8 QueueSample(i16 sample, ChannelId ch_id);
		void BufferFull();

		static_assert(sizeof(m_soundcnt_h) == 2);
		static_assert(sizeof(m_soundcnt_x) == 1);
		static_assert(sizeof(m_soundbias) == 2);
	};
}