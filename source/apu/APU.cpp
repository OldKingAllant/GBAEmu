#include "../../apu/APU.hpp"

#include "../../memory/MMIO.hpp"
#include "../../common/BitManip.hpp"

#include "../../memory/DirectMemoryAccess.hpp"
#include "../../memory/EventScheduler.hpp"

#include "../../common/Error.hpp"

#include <bit>
#include <algorithm>

namespace GBA::apu {
	void output_sample(void* userdata);

	APU::APU() :
		m_sample_buffer{nullptr}, m_dma1{nullptr},
		m_dma2{nullptr}, m_buffer_callback{},
		m_curr_samples{}, m_req_samples{}, 
		m_internal_A_buffer{}, m_internal_B_buffer{},
		m_A_pos{}, m_B_pos {},
		m_soundcnt_h{},
		m_soundcnt_x{}, m_soundbias{},
		m_interleaved_buffer_pos_l{}, 
		m_interleaved_buffer_pos_r{},
		m_freq{}, m_cycles_before_output{},
		m_curr_ch_samples{}, m_curr_ch_sample_accum{},
		m_sched(nullptr)
	{}

	void APU::SetDma(memory::DMA* _1, memory::DMA* _2) {
		m_dma1 = _1;
		m_dma2 = _2;
	}

	void APU::SetCallback(SampleCallback callback, u32 required_samples) {
		m_buffer_callback = callback;
		m_req_samples = required_samples;

		m_sample_buffer = new i16[required_samples * 2];

		std::fill_n(m_sample_buffer, required_samples * 2, 0x00);
	}

	void APU::Clock(u32 num_cycles) {
		(void)num_cycles;
	}

	void APU::SetMMIO(memory::MMIO* mmio) {
		u8* a_buf = std::bit_cast<u8*>((i8*)m_internal_A_buffer);

		mmio->AddRegister<u32>(0xA0, false, true, a_buf, 0xFFFF,
			[this](u8 data, u16 offset) {
				offset -= 0xA0;
				if (m_A_pos < 32)
					m_internal_A_buffer[m_A_pos++] = data;
		});

		u8* b_buf = std::bit_cast<u8*>((i8*)m_internal_A_buffer);

		mmio->AddRegister<u32>(0xA4, false, true, b_buf, 0xFFFF,
			[this](u8 data, u16 offset) {
				offset -= 0xA4;
				if(m_B_pos < 32)
					m_internal_B_buffer[m_B_pos++] = data;
		});

		u8* soundcnt_h = std::bit_cast<u8*>(&m_soundcnt_h);

		mmio->AddRegister<u16>(0x82, true, true, soundcnt_h, 0xFFFF,
			[this](u8 data, u16 offset) {
				offset -= 0x82;

				u8* soundcnt_h = std::bit_cast<u8*>(&m_soundcnt_h);

				soundcnt_h[offset] = data;

				if (m_soundcnt_h.fifo_a_reset) {
					m_soundcnt_h.fifo_a_reset = 0;
					m_A_pos = 0;
				}

				if (m_soundcnt_h.fifo_b_reset) {
					m_soundcnt_h.fifo_b_reset = 0;
					m_B_pos = 0;
				}
			});

		u8* master_control = std::bit_cast<u8*>(&m_soundcnt_x);

		mmio->AddRegister<u8>(0x84, true, true, master_control, 0x80,
			[this](u8 data, u16 pos) {
				m_soundcnt_x.master_en = CHECK_BIT(data, 7);

				if (!m_soundcnt_x.master_en)
					std::fill_n(m_curr_ch_samples, 6, 0x0);
			});

		u8* bias_cnt = std::bit_cast<u8*>(&m_soundbias);

		mmio->AddRegister<u16>(0x88, true, true, bias_cnt, 0xFFFF);
	}

	void APU::SetFreq(u32 freq) {
		m_freq = freq;
		m_cycles_before_output = CPU_FREQ / 32768;

		m_sched->Schedule(256, memory::EventType::APU_SAMPLE_OUT,
			output_sample, std::bit_cast<void*>(this));
	}

	void APU::SetScheduler(memory::EventScheduler* sched) {
		m_sched = sched;
	}

	void APU::MixSample(i16& sample_l, i16& sample_r, ChannelId ch_id) {
		switch (ch_id)
		{
		case GBA::apu::ChannelId::FIFO_A: {
			if (m_soundcnt_h.fifo_a_left) {
				sample_l += m_curr_ch_samples[0];
			}

			if (m_soundcnt_h.fifo_a_right) {
				sample_r += m_curr_ch_samples[0];
			}
		}
			break;
		case GBA::apu::ChannelId::FIFO_B: {
			if (m_soundcnt_h.fifo_b_left) {
				sample_l += m_curr_ch_samples[1];
			}

			if (m_soundcnt_h.fifo_b_right) {
				sample_r += m_curr_ch_samples[1];
			}
		}
			break;
		default:
			error::DebugBreak();
			break;
		}
	}

	void APU::BufferFull() {
		/*
		u8 bias_low : 7;
		u8 : 0;
		u8 bias_high : 2;*/

		if (!m_sample_buffer)
			return;

		m_curr_samples = 0;
		m_interleaved_buffer_pos_l = 0;
		m_interleaved_buffer_pos_r = 0;
	}

	void APU::TimerOverflow(u8 id) {
		if (!m_soundcnt_x.master_en)
			return;

		if (m_soundcnt_h.fifo_a_timer_sel == id) {
			if (m_sample_buffer) {
				i16 sample = m_A_pos ? m_internal_A_buffer[0] : 0x0;

				if (!m_soundcnt_h.fifo_a_volume)
					sample >>= 2;

				m_curr_ch_samples[0] = sample;

				std::shift_left(std::begin(m_internal_A_buffer),
					std::end(m_internal_A_buffer), 1);

				m_A_pos--;
			}

			if (m_A_pos <= 16) {
				m_dma1->TriggerDMA(memory::DMAFireType::FIFO_A);
				m_dma2->TriggerDMA(memory::DMAFireType::FIFO_A);
			}
		}
		
		if (m_soundcnt_h.fifo_b_timer_sel == id) {
			if (m_sample_buffer) {
				i16 sample = m_B_pos ? m_internal_B_buffer[0] : 0x0;

				if (!m_soundcnt_h.fifo_b_volume)
					sample >>= 2;

				m_curr_ch_samples[1] = sample;

				std::shift_left(std::begin(m_internal_B_buffer),
					std::end(m_internal_B_buffer), 1);

				m_B_pos--;
			}

			if (m_B_pos <= 16) {
				m_dma1->TriggerDMA(memory::DMAFireType::FIFO_B);
				m_dma2->TriggerDMA(memory::DMAFireType::FIFO_B);
			}
		}
	}

	void output_sample(void* userdata) {
		APU* apu = std::bit_cast<APU*>(userdata);

		i16 left_sample = 0;
		i16 right_sample = 0;

		apu->MixSample(left_sample, right_sample, ChannelId::FIFO_A);
		apu->MixSample(left_sample, right_sample, ChannelId::FIFO_B);

		u32 bias = apu->m_soundbias.bias_low | (apu->m_soundbias.bias_high << 7);

		left_sample += 0x200;
		left_sample = std::clamp(left_sample, (i16)0x0, (i16)0x3FF);
		left_sample -= 0x200;
		left_sample *= 128;

		right_sample += 0x200;
		right_sample = std::clamp(right_sample, (i16)0x0, (i16)0x3FF);
		right_sample -= 0x200;
		right_sample *= 128;

		//apu->m_buffer_callback(left_sample, right_sample);

		apu->m_curr_samples++;

		if (apu->m_curr_samples == apu->m_req_samples) {
			apu->BufferFull();
		}

		u32 cycles = 512 -
			(apu->m_sched->GetTimestamp() & 511);

		apu->m_sched->Schedule(cycles, memory::EventType::APU_SAMPLE_OUT,
			output_sample, userdata, true);
	}

	APU::~APU() {
		if (m_sample_buffer)
			delete[] m_sample_buffer;
	}
}