#include "../../apu/APU.hpp"

#include "../../memory/MMIO.hpp"
#include "../../common/BitManip.hpp"

#include "../../memory/DirectMemoryAccess.hpp"
#include "../../memory/EventScheduler.hpp"

#include "../../common/Error.hpp"
#include "../../common/Logger.hpp"

#include "../../apu/SquareChannel.hpp"
#include "../../apu/NoiseChannel.hpp"

#include <bit>
#include <algorithm>

namespace GBA::apu {
	void output_sample(void* userdata);

	APU::APU() :
		m_dma1{nullptr}, m_dma2{nullptr},
		m_buffer_callback{},
		m_curr_samples{}, m_req_samples{}, 
		m_internal_A_buffer{}, m_internal_B_buffer{},
		m_A_pos{}, m_B_pos {},
		m_soundcnt_h{},
		m_soundcnt_x{}, m_soundbias{},
		m_freq{}, m_cycles_before_output{},
		m_curr_ch_samples{}, m_curr_ch_sample_accum{},
		m_sched(nullptr),
		m_sound1_freq_control{}, m_sound2_freq_control{},
		m_sound3_freq_control{}, m_sound4_freq_control{},
		m_sound1{nullptr}, m_sound2{nullptr}, m_noise{nullptr},
		m_soundcnt_l{}
	{
		m_soundbias.raw = 0x200;

		m_sound1 = new SquareChannel(true);
		m_sound2 = new SquareChannel(false);
		m_noise = new NoiseChannel();
	}

	void APU::SetDma(memory::DMA* _1, memory::DMA* _2) {
		m_dma1 = _1;
		m_dma2 = _2;
	}

	void APU::SetCallback(SampleCallback callback, u32 required_samples) {
		m_buffer_callback = callback;
		m_req_samples = required_samples;
	}

	void APU::Clock(u32 num_cycles) {
		(void)num_cycles;
	}

	void APU::SetMMIO(memory::MMIO* mmio) {
		u8* sound1_cnt = std::bit_cast<u8*>(&m_sound1_freq_control);
		u8* sound2_cnt = std::bit_cast<u8*>(&m_sound2_freq_control);
		u8* sound3_cnt = std::bit_cast<u8*>(&m_sound3_freq_control);
		u8* sound4_cnt = std::bit_cast<u8*>(&m_sound4_freq_control);

		m_sound1->SetMMIO(mmio);
		m_sound2->SetMMIO(mmio);
		m_noise->SetMMIO(mmio);

		m_sound1->SetEnableCallback([this](bool en) {
			m_soundcnt_x.sound_1_on = en;
		});

		m_sound2->SetEnableCallback([this](bool en) {
			m_soundcnt_x.sound_2_on = en;
		});

		m_noise->SetEnableCallback([this](bool en) {
			m_soundcnt_x.sound_4_on = en;
		});

		mmio->AddRegister<u16>(0x74, true, true, sound3_cnt, 0xFFFF, [this](u8 value, u16) {
			logging::Logger::Instance().LogInfo("APU", " Writing sound3 control 0x{:x}", value);
		});

		/*mmio->AddRegister<u16>(0x7C, true, true, sound4_cnt, 0xFFFF, [this](u8 value, u16) {
			logging::Logger::Instance().LogInfo("APU", " Writing sound4 control 0x{:x}", value);
		});*/

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

		u8* soundcnt_l = std::bit_cast<u8*>(&m_soundcnt_l);

		mmio->AddRegister<u16>(0x80, true, true, soundcnt_l, 0xFFFF);

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

				if (!m_soundcnt_x.master_en) {
					std::fill_n(m_curr_ch_samples, 6, 0x0);
					std::fill_n(m_curr_ch_sample_accum, 6, 0x0);
				}
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
		m_sound1->SetScheduler(sched);
		m_sound2->SetScheduler(sched);
		m_noise->SetScheduler(sched);
	}

	void APU::MixSample(i16& sample_l, i16& sample_r, ChannelId ch_id) {
		u32 ch_a_accum = m_curr_ch_sample_accum[0] ? m_curr_ch_sample_accum[0] : 1;
		u32 ch_b_accum = m_curr_ch_sample_accum[1] ? m_curr_ch_sample_accum[1] : 1;

		u8 vol_l = m_soundcnt_l.dmg_sound_vol_l;
		u8 vol_r = m_soundcnt_l.dmg_sound_vol_r;

		u8 dmg_sound_vol = m_soundcnt_h.dmg_sound_volume;

		switch (ch_id)
		{
		case GBA::apu::ChannelId::FIFO_A: {
			if (m_soundcnt_h.fifo_a_left) {
				sample_l += m_curr_ch_samples[0] / ch_a_accum;
			}

			if (m_soundcnt_h.fifo_a_right) {
				sample_r += m_curr_ch_samples[0] / ch_a_accum;
			}
		}
		break;
		case GBA::apu::ChannelId::FIFO_B: {
			if (m_soundcnt_h.fifo_b_left) {
				sample_l += m_curr_ch_samples[1] / ch_b_accum;
			}

			if (m_soundcnt_h.fifo_b_right) {
				sample_r += m_curr_ch_samples[1] / ch_b_accum;
			}
		}
		break;
		case GBA::apu::ChannelId::PULSE_1: {
			if (!m_soundcnt_x.sound_1_on)
				return;

			i16 sample = m_sound1->GetSample() / m_sound1->GetNumAccum();
			m_sound1->ResetAccum();

			if (m_soundcnt_l.snd1_en_l) {
				i16 mod_sample = sample * vol_l;

				if (dmg_sound_vol < 3) {
					mod_sample >>= (2 - dmg_sound_vol);
				}

				mod_sample /= 2;

				sample_l += mod_sample;
			}

			if (m_soundcnt_l.snd1_en_r) {
				i16 mod_sample = sample * vol_r;

				if (dmg_sound_vol < 3) {
					mod_sample >>= (2 - dmg_sound_vol);
				}

				mod_sample /= 2;

				sample_r += mod_sample;
			}
		}
		break;
		case GBA::apu::ChannelId::PULSE_2: {
			if (!m_soundcnt_x.sound_2_on)
				return;

			i16 sample = m_sound2->GetSample() / m_sound2->GetNumAccum();
			m_sound2->ResetAccum();

			if (m_soundcnt_l.snd2_en_l) {
				i16 mod_sample = sample * vol_l;

				if (dmg_sound_vol < 3) {
					mod_sample >>= (2 - dmg_sound_vol);
				}

				mod_sample /= 3;

				sample_l += mod_sample;
			}

			if (m_soundcnt_l.snd2_en_r) {
				i16 mod_sample = sample * vol_r;

				if (dmg_sound_vol < 3) {
					mod_sample >>= (2 - dmg_sound_vol);
				}

				mod_sample /= 3;

				sample_r += mod_sample;
			}
		}
		break;
		case GBA::apu::ChannelId::NOISE: {
			if (!m_soundcnt_x.sound_4_on)
				return;

			i16 sample = m_noise->GetSample() / m_noise->GetNumAccum();
			m_noise->ResetAccum();

			if (m_soundcnt_l.snd4_en_l) {
				i16 mod_sample = sample * vol_l;

				if (dmg_sound_vol < 3) {
					mod_sample >>= (2 - dmg_sound_vol);
				}

				mod_sample /= 3;

				sample_l += mod_sample;
			}

			if (m_soundcnt_l.snd4_en_r) {
				i16 mod_sample = sample * vol_r;

				if (dmg_sound_vol < 3) {
					mod_sample >>= (2 - dmg_sound_vol);
				}

				mod_sample /= 3;

				sample_r += mod_sample;
			}
		}
		break;
		default:
			error::DebugBreak();
			break;
		}
	}

	void APU::BufferFull() {
		m_curr_samples = 0;
	}

	void APU::TimerOverflow(u8 id) {
		if (!m_soundcnt_x.master_en)
			return;

		if (m_soundcnt_h.fifo_a_timer_sel == id) {
			i16 sample = m_A_pos ? m_internal_A_buffer[0] : 0x0;

			if (!m_soundcnt_h.fifo_a_volume)
				sample >>= 2;

			if (m_curr_ch_sample_accum[0] == 0)
				m_curr_ch_samples[0] = sample;
			else
				m_curr_ch_samples[0] += sample;

			m_curr_ch_sample_accum[0]++;

			std::shift_left(std::begin(m_internal_A_buffer),
				std::end(m_internal_A_buffer), 1);

			if(m_A_pos)
				m_A_pos--;

			if (m_A_pos <= 16) {
				m_dma1->TriggerDMA(memory::DMAFireType::FIFO_A);
				m_dma2->TriggerDMA(memory::DMAFireType::FIFO_A);
			}
		}
		
		if (m_soundcnt_h.fifo_b_timer_sel == id) {
			i16 sample = m_B_pos ? m_internal_B_buffer[0] : 0x0;

			if (!m_soundcnt_h.fifo_b_volume)
				sample >>= 2;

			if (m_curr_ch_sample_accum[1] == 0)
				m_curr_ch_samples[1] = sample;
			else
				m_curr_ch_samples[1] += sample;

			m_curr_ch_sample_accum[1]++;

			std::shift_left(std::begin(m_internal_B_buffer),
				std::end(m_internal_B_buffer), 1);

			if(m_B_pos)
				m_B_pos--;

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
		apu->MixSample(left_sample, right_sample, ChannelId::PULSE_1);
		apu->MixSample(left_sample, right_sample, ChannelId::PULSE_2);
		apu->MixSample(left_sample, right_sample, ChannelId::NOISE);

		u32 bias = apu->m_soundbias.bias_low | (apu->m_soundbias.bias_high << 7);

		u32 amplification = apu->SampleAmplification();

		left_sample += bias;
		left_sample = std::clamp(left_sample, (i16)0x0, (i16)0x3FF);
		left_sample -= bias;
		left_sample *= amplification;

		right_sample += bias;
		right_sample = std::clamp(right_sample, (i16)0x0, (i16)0x3FF);
		right_sample -= bias;
		right_sample *= amplification;

		apu->m_buffer_callback(left_sample, right_sample);

		std::fill_n(apu->m_curr_ch_sample_accum, 6, 0x0);

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
		delete m_sound1;
		delete m_sound2;
	}
}