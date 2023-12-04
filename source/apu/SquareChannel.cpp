#include "../../apu/SquareChannel.hpp"

#include "../../memory/EventScheduler.hpp"
#include "../../memory/MMIO.hpp"

namespace GBA::apu {
	SquareChannel::SquareChannel(bool has_sweep)
		: m_envelope_control{},
		m_control{}, m_has_sweep{has_sweep}, 
		m_sched{nullptr}, m_curr_sample{},
		m_curr_wave_pos{}, m_curr_freq{},
		m_enabled{}, m_en_callback{},
		m_sample_accum{1}, m_seq{has_sweep}
	{}

	void seq_update(void* userdata) {
		SquareChannel* ch = std::bit_cast<SquareChannel*>(userdata);

		uint64_t timestamp = ch->m_sched->GetTimestamp();

		SeqEvent ev = ch->m_seq.Update(timestamp);

		bool stopped = false;

		if (ev & SWEEP_END) {
			stopped = true;
		}

		if ((ev & LEN_EXPIRED) && ch->m_control.stop_on_len)
			stopped = true;

		memory::EventType event_tp = ch->m_has_sweep ?
			memory::EventType::APU_CH1_SEQUENCER :
			memory::EventType::APU_CH2_SEQUENCER;

		if (!stopped) {
			ch->m_sched->Schedule((u32)SquareChannel::LEN_CYCLES, event_tp, seq_update,
				userdata, true);
		}
		else {
			ch->m_enabled = false;
			ch->m_en_callback(false);
		}
	}

	void sample_update(void* userdata) {
		SquareChannel* ch = std::bit_cast<SquareChannel*>(userdata);

		if (!ch->m_enabled)
			return;

		memory::EventType event_tp = ch->m_has_sweep ?
			memory::EventType::APU_CH1_SAMPLE_UPDATE :
			memory::EventType::APU_CH2_SAMPLE_UPDATE;

		u32 freq = ch->m_has_sweep ? ch->m_seq.GetFreq() : (2048 - ch->m_curr_freq);
		u32 cycles = (u32)(SquareChannel::CPU_CYCLES / freq);

		ch->m_sched->Schedule(cycles, event_tp, sample_update, userdata, true);
	}

	void SquareChannel::SetMMIO(memory::MMIO* mmio) {
		if (m_has_sweep) {
			u8* sweep = std::bit_cast<u8*>(&m_seq.m_sweep->m_control);

			mmio->AddRegister<u16>(0x60, true, true, sweep, 0xFF);
		}

		u32 envelope_address = m_has_sweep ? 0x62 : 0x68;
		u32 control_address = m_has_sweep ? 0x64 : 0x6C;

		u8* env = std::bit_cast<u8*>(&m_envelope_control);
		u8* cnt = std::bit_cast<u8*>(&m_control);

		mmio->AddRegister<u16>(envelope_address, true, true, env, 0xFFFF,
			[this, envelope_address](u8 value, u16 offset) {
				offset -= envelope_address;

				u8* env_cnt = std::bit_cast<u8*>(&m_envelope_control);

				env_cnt[offset] = value;

				m_seq.m_envelope.SetStepTime(m_envelope_control.time);
				m_seq.m_envelope.SetDirection(m_envelope_control.direction);
				m_seq.m_envelope.SetInitVol(m_envelope_control.init_vol);
		});

		mmio->AddRegister<u16>(control_address, true, true, cnt, 0xFFFF,
			[this, control_address](u8 value, u16 offset) {
				offset -= control_address;

				if (offset == 0) {
					m_curr_freq &= ~((u16)0xFF);
					m_curr_freq |= value;
					return;
				}

				m_curr_freq &= ~((u16)0b111 << 8);
				m_curr_freq |= ((u16)(value & 0b111) << 8);

				m_control.stop_on_len = CHECK_BIT(value, 6);
				m_seq.m_enable_len_counter = m_control.stop_on_len;

				if (CHECK_BIT(value, 7))
					Restart();
		});
	}

	void SquareChannel::SetScheduler(memory::EventScheduler* sched) {
		m_sched = sched;
	}

	void SquareChannel::SetEnableCallback(EnableCallback callback) {
		m_en_callback = callback;
	}

	i16 SquareChannel::GetSample() const {
		return (i16)m_curr_sample * m_seq.m_envelope.IsDacOn();
	}

	void SquareChannel::Restart() {
		m_enabled = true;

		u32 freq = 2048 - m_curr_freq;
		u32 cycles = (u32)(CPU_CYCLES / freq);

		m_seq.Restart(freq);

		if (m_has_sweep) {
			m_sched->Deschedule(memory::EventType::APU_CH1_SEQUENCER);
			m_sched->Deschedule(memory::EventType::APU_CH1_SAMPLE_UPDATE);

			m_sched->Schedule((u32)LEN_CYCLES, memory::EventType::APU_CH1_SEQUENCER, seq_update,
				std::bit_cast<void*>(this));

			m_sched->Schedule(cycles, memory::EventType::APU_CH1_SAMPLE_UPDATE, sample_update,
				std::bit_cast<void*>(this));
		}
		else {
			m_sched->Deschedule(memory::EventType::APU_CH2_SEQUENCER);
			m_sched->Deschedule(memory::EventType::APU_CH2_SAMPLE_UPDATE);

			m_sched->Schedule((u32)LEN_CYCLES, memory::EventType::APU_CH2_SEQUENCER, seq_update,
				std::bit_cast<void*>(this));

			m_sched->Schedule(cycles, memory::EventType::APU_CH2_SAMPLE_UPDATE, sample_update,
				std::bit_cast<void*>(this));
		}

		if (m_en_callback)
			m_en_callback(true);
	}

	i16 SquareChannel::GetNumAccum() const {
		return m_sample_accum;
	}

	void SquareChannel::ResetAccum() {
		m_sample_accum = 1;
	}
}