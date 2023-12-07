#pragma once

#include "../common/Defs.hpp"

#include <functional>

namespace GBA {
	namespace memory {
		class DMA;
		class MMIO;
		class EventScheduler;
	}
}

namespace GBA::apu {
	using namespace common;

	class SquareChannel;
	class NoiseChannel;
	class WaveChannel;

	using SampleCallback = std::function<void(i16, i16)>;

	enum class ChannelId {
		FIFO_A,
		FIFO_B,
		PULSE_1,
		PULSE_2,
		WAVE,
		NOISE
	};

	class APU {
	public :
		APU();

		void SetDma(memory::DMA* _1, memory::DMA* _2);
		void SetCallback(SampleCallback callback, u32 required_samples);

		void Clock(u32 num_cycles);

		void SetMMIO(memory::MMIO* mmio);
		void SetScheduler(memory::EventScheduler* sched);

		void TimerOverflow(u8 id);

		void SetFreq(u32 freq);

		~APU();

	private :
		memory::DMA* m_dma1;
		memory::DMA* m_dma2;

		SampleCallback m_buffer_callback;
		u32 m_curr_samples;
		u32 m_req_samples;

		i8 m_internal_A_buffer[32];
		i8 m_internal_B_buffer[32];
		i8 m_A_pos;
		i8 m_B_pos;

		u32 m_freq;
		u32 m_cycles_before_output;

		i16 m_curr_ch_samples[6];
		u32 m_curr_ch_sample_accum[6];
		
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

		inline u32 SampleAmplification() const {
			return (512 >> m_soundbias.amplitude);
		}

		memory::EventScheduler* m_sched;

		u16 m_sound1_freq_control;
		u16 m_sound2_freq_control;
		u16 m_sound3_freq_control;
		u16 m_sound4_freq_control;

		SquareChannel* m_sound1;
		SquareChannel* m_sound2;
		NoiseChannel* m_noise;
		WaveChannel* m_wave;

		union {
			struct {
				u8 dmg_sound_vol_r : 3;
				u8 : 1;
				u8 dmg_sound_vol_l : 3;
				u8 : 1;
				bool snd1_en_r : 1;
				bool snd2_en_r : 1;
				bool snd3_en_r : 1;
				bool snd4_en_r : 1;
				//
				bool snd1_en_l : 1;
				bool snd2_en_l : 1;
				bool snd3_en_l : 1;
				bool snd4_en_l : 1;
			};

			u16 raw;
		} m_soundcnt_l;

		void MixSample(i16& sample_l, i16& sample_r, ChannelId ch_id);
		void BufferFull();

		static_assert(sizeof(m_soundcnt_h) == 2);
		static_assert(sizeof(m_soundcnt_x) == 1);
		static_assert(sizeof(m_soundbias) == 2);

		static constexpr uint64_t CPU_FREQ = 16'780'000;

		friend void output_sample(void* userdata);
	};
}