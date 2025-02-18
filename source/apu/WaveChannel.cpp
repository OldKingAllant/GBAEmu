#include "../../apu/WaveChannel.hpp"

#include "../../memory/EventScheduler.hpp"
#include "../../memory/MMIO.hpp"

namespace GBA::apu {
	WaveChannel::WaveChannel() :
		m_control_l{}, m_control_h{},
		m_freq_control{}, m_freq{},
		m_enable{false}, m_wave_ram{},
		m_curr_len{}, m_curr_div_value{},
		m_curr_sample_pos{},
		m_curr_bank{}
	{}

	void wave_seq_update(void* userdata) {
		WaveChannel* ch = std::bit_cast<WaveChannel*>(userdata);

		if (ch->m_curr_div_value && ch->m_freq_control.stop_on_len) {
			ch->m_curr_len++;

			if (ch->m_curr_len >= 256) {
				ch->m_enable = false;
				ch->m_en_callback(false);
			}
		}

		ch->m_curr_div_value = (ch->m_curr_div_value + 1) % 2;

		if(ch->m_enable)
			ch->m_sched->Schedule((u32)WaveChannel::DIV_CYCLES, memory::EventType::APU_CH3_SEQUENCER,
				wave_seq_update, userdata, true);
		else {
			ch->m_sched->Deschedule(memory::EventType::APU_CH3_SAMPLE_UPDATE);
			ch->m_curr_sample = 0;
			ch->m_sample_accum = 1;
		}
	}

	void wave_sample_update(void* userdata) {
		WaveChannel* ch = std::bit_cast<WaveChannel*>(userdata);

		u32 freq = (u32)(WaveChannel::BASE_HZ / ((uint64_t)2048 - ch->m_freq));
		u32 cycles = (u32)(WaveChannel::CPU_CYCLES / freq);

		u8 digit = ch->m_wave_ram[ch->m_curr_bank][ch->m_curr_sample_pos / 2];

		if (ch->m_curr_sample_pos & 1)
			digit &= 0xF;
		else
			digit = (digit >> 4) & 0xF;

		ch->m_curr_sample_pos++;

		if (ch->m_curr_sample_pos >= 32) {
			ch->m_curr_sample_pos = 0;

			if (ch->m_control_l.ram_dimension)
				ch->m_curr_bank = 1 - ch->m_curr_bank;
		}

		if (ch->m_control_h.force_75)
			digit = (u8)(digit * 0.75);
		else {
			if (ch->m_control_h.volume != 0) {
				digit >>= ch->m_control_h.volume - 1;
			}
		}

		if (ch->m_sample_accum == 1) {
			ch->m_curr_sample = digit;
		}
		else {
			ch->m_curr_sample += digit;
		}

		ch->m_sample_accum++;

		if(ch->m_enable)
			ch->m_sched->Schedule(cycles, memory::EventType::APU_CH3_SAMPLE_UPDATE,
				wave_sample_update, userdata, true);
	}

	void WaveChannel::SetMMIO(memory::MMIO* mmio) {
		u8* m_cnt_l = std::bit_cast<u8*>(&m_control_l);

		mmio->AddRegister<u16>(0x70, true, true, m_cnt_l, 0xFFFF,
			[this](u8 value, u16 offset) {
				offset -= 0x70;

				if (offset)
					return;

				m_control_l.ram_dimension = CHECK_BIT(value, 5);
				m_control_l.bank_number = CHECK_BIT(value, 6);

				if (!CHECK_BIT(value, 7) && m_control_l.dac_enable) {
					m_sched->Deschedule(memory::EventType::APU_CH3_SEQUENCER);
					m_sched->Deschedule(memory::EventType::APU_CH3_SAMPLE_UPDATE);

					if (m_en_callback)
						m_en_callback(false);

					m_enable = false;

					m_curr_sample = 0;
					m_sample_accum = 1;
				}

				m_control_l.dac_enable = CHECK_BIT(value, 7);

				m_curr_bank = m_control_l.bank_number;
				m_curr_sample_pos = 0;
		});

		u8* m_cnt_h = std::bit_cast<u8*>(&m_control_h);

		mmio->AddRegister<u16>(0x72, true, true, m_cnt_h, 0xFFFF);

		u8* freq_cnt = std::bit_cast<u8*>(&m_freq_control);

		mmio->AddRegister<u16>(0x74, true, true, freq_cnt, 0xFFFF,
			[this](u8 value, u16 offset) {
				offset -= 0x74;

				if (offset == 0) {
					m_freq &= ~(u32)0xFF;
					m_freq |= value;
					return;
				}

				m_freq &= ~((u32)0b111 << 8);
				m_freq |= (((u32)value & 0b111) << 8);

				m_freq_control.stop_on_len = CHECK_BIT(value, 6);

				if (CHECK_BIT(value, 7))
					Restart();
		});

		for (u32 start = 0x90; start <= 0x9F; start++) {
			mmio->AddRegister<u8>(start, true, true, &m_wave_ram[0][0],
				0xFF, [this](u8 value, u16 offset) {
					m_wave_ram[!m_control_l.bank_number][offset - 0x90] = value;
			});
		}
	}

	void WaveChannel::RegisterEventTypes() {
		m_sched->SetEventTypeRodata(memory::EventType::APU_CH3_SAMPLE_UPDATE,
			wave_sample_update, std::bit_cast<void*>(this));
		m_sched->SetEventTypeRodata(memory::EventType::APU_CH3_SEQUENCER,
			wave_seq_update, std::bit_cast<void*>(this));
	}

	void WaveChannel::Restart() {
		m_sched->Deschedule(memory::EventType::APU_CH3_SEQUENCER);
		m_sched->Deschedule(memory::EventType::APU_CH3_SAMPLE_UPDATE);

		m_curr_sample = 0;
		m_sample_accum = 1;

		if (!m_control_l.dac_enable)
			return;

		m_enable = true;

		if (m_en_callback)
			m_en_callback(true);

		m_curr_len = 256 - m_control_h.len;
		m_curr_sample_pos = 0;
		m_curr_bank = m_control_l.bank_number;

		m_sched->Schedule((u32)DIV_CYCLES, memory::EventType::APU_CH3_SEQUENCER,
			wave_seq_update, std::bit_cast<void*>(this));

		u32 freq = (u32)(BASE_HZ / ((uint64_t)2048 - m_freq));
		u32 cycles = (u32)(CPU_CYCLES / freq);

		m_sched->Schedule(cycles, memory::EventType::APU_CH3_SAMPLE_UPDATE,
			wave_sample_update, std::bit_cast<void*>(this));
	}
}