#include "../../apu/NoiseChannel.hpp"

#include "../../memory/EventScheduler.hpp"
#include "../../memory/MMIO.hpp"

namespace GBA::apu {
	NoiseChannel::NoiseChannel() : m_seq(false),
		m_enabled{ false }, m_envelope_control{},
		m_control{}, m_lsfr{0x4000}
	{}

	void noise_seq_update(void* userdata) {
		NoiseChannel* ch = std::bit_cast<NoiseChannel*>(userdata);

		uint64_t timestamp = ch->m_sched->GetTimestamp();

		SeqEvent ev = ch->m_seq.Update(timestamp);

		bool stopped = false;

		if ((ev & LEN_EXPIRED) && ch->m_control.stop_on_len)
			stopped = true;

		if (!stopped) {
			ch->m_sched->Schedule((u32)NoiseChannel::DIV_CYCLES, memory::EventType::APU_CH4_SEQUENCER, 
				noise_seq_update, userdata, true);
		}
		else {
			ch->m_enabled = false;
			ch->m_en_callback(false);

			ch->m_sched->Deschedule(memory::EventType::APU_CH4_SAMPLE_UPDATE);
			ch->m_curr_sample = 0;
			ch->m_sample_accum = 1;
		}
	}

	void noise_sample_update(void* userdata) {
		NoiseChannel* ch = std::bit_cast<NoiseChannel*>(userdata);

		if (!ch->m_enabled)
			return;

		/*
		  7bit:  X=X SHR 1, IF carry THEN Out=HIGH, X=X XOR 60h ELSE Out=LOW
		  15bit: X=X SHR 1, IF carry THEN Out=HIGH, X=X XOR 6000h ELSE Out=LOW
		*/
		u8 bit = ch->m_lsfr & 1;
		ch->m_lsfr >>= 1;

		if (bit) {
			if (ch->m_control.counter_width) {
				//7 bits
				ch->m_lsfr = ch->m_lsfr ^ 0x60;
			}
			else {
				//15 bits
				ch->m_lsfr = ch->m_lsfr ^ 0x6000;
			}
		}

		i16 sample = (i16)bit * ch->m_seq.GetVolume();

		if (ch->m_sample_accum == 1) {
			ch->m_curr_sample = sample;
		}
		else {
			ch->m_curr_sample += sample;
		}

		ch->m_sample_accum++;

		double r = ch->m_control.freq_div ? ch->m_control.freq_div : 0.5;
		u32 shift = (u32)ch->m_control.shift_freq + 1;
		double s = (double)((uint64_t)2 << shift);
		u32 computed_freq = (u32)(NoiseChannel::BASE_HZ / r / s);
		u32 cycles = (u32)NoiseChannel::CPU_CYCLES / computed_freq;

		ch->m_sched->Schedule(cycles, memory::EventType::APU_CH4_SAMPLE_UPDATE,
			noise_sample_update, userdata, true);
	}

	void NoiseChannel::SetMMIO(memory::MMIO* mmio) {
		u8* env = std::bit_cast<u8*>(&m_envelope_control);
		u8* cnt = std::bit_cast<u8*>(&m_control);

		mmio->AddRegister<u16>(0x78, true, true, env, 0xFFFF,
			[this](u8 value, u16 offset) {
				offset -= 0x78;

				u8* env_cnt = std::bit_cast<u8*>(&m_envelope_control);

				env_cnt[offset] = value;

				m_seq.m_envelope.SetStepTime(m_envelope_control.envelope_step);
				m_seq.m_envelope.SetDirection(m_envelope_control.envelope_dir);
				m_seq.m_envelope.SetInitVol(m_envelope_control.envelope_init);
				m_seq.m_len_counter.SetLen(m_envelope_control.len);
			});

		mmio->AddRegister<u16>(0x7C, true, true, cnt, 0xFFFF,
			[this](u8 value, u16 offset) {
				offset -= 0x7C;

				if (offset == 0) {
					m_control.freq_div = value & 0x7;
					m_control.counter_width = CHECK_BIT(value, 3);
					m_control.shift_freq = (value >> 4) & 0xF;
					return;
				}

				m_control.stop_on_len = CHECK_BIT(value, 6);
				m_seq.m_enable_len_counter = m_control.stop_on_len;

				if (CHECK_BIT(value, 7))
					Restart();
			});
	}

	void NoiseChannel::Restart() {
		m_enabled = true;

		if (m_control.counter_width) {
			m_lsfr = 0x40;
		}
		else {
			m_lsfr = 0x4000;
		}

		m_seq.Restart(0);
		m_seq.m_len_counter.SetLen(m_envelope_control.len);

		m_sched->Deschedule(memory::EventType::APU_CH4_SEQUENCER);
		m_sched->Deschedule(memory::EventType::APU_CH4_SAMPLE_UPDATE);

		m_sched->Schedule((u32)DIV_CYCLES, memory::EventType::APU_CH4_SEQUENCER,
			noise_seq_update, std::bit_cast<void*>(this));

		double r = m_control.freq_div ? m_control.freq_div : 0.5;
		u32 shift = (u32)m_control.shift_freq + 1;
		double s = (double)((uint64_t)2 << shift);
		u32 computed_freq = (u32)(BASE_HZ / r / s);
		u32 cycles = (u32)CPU_CYCLES / computed_freq;

		m_sched->Schedule(cycles, memory::EventType::APU_CH4_SAMPLE_UPDATE,
			noise_sample_update, std::bit_cast<void*>(this));

		if (m_en_callback)
			m_en_callback(true);
	}
}