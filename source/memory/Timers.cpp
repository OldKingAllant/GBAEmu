#include "../../memory/Timers.hpp"

#include "../../memory/MMIO.hpp"
#include "../../memory/InterruptController.hpp"
#include "../../memory/EventScheduler.hpp"

#include "../../common/BitManip.hpp"

#include "../../common/Logger.hpp"

namespace GBA::timers {
	LOG_CONTEXT(Timers);

	using namespace common;

	TimerChain::TimerChain() :
		m_int_controller(nullptr), m_sched(nullptr), 
		m_registers{}, m_timer_reload_val{},
		m_timer_internal_counter{}
	{}

	void TimerChain::SetInterruptController(memory::InterruptController* int_control) {
		m_int_controller = int_control;
	}

	void TimerChain::SetEventScheduler(memory::EventScheduler* ev_sched) {
		m_sched = ev_sched;
	}

	void TimerChain::SetMMIO(memory::MMIO* mmio) {
		mmio->AddRegister<u16>(TIMER_REG_BASE, true, true, &m_registers[0x0], 0xFFFF, [this](u8 value, u16 offset) {
			u8 shift_amount = offset % 2;
			u16 modify = ((u16)value << (8 * shift_amount));

			u16 curr_value = m_timer_reload_val[0];

			curr_value &= ~((u16)0xFF << (8 * shift_amount));
			curr_value |= modify;

			m_timer_reload_val[0] = curr_value;
		});

		mmio->AddRegister<u16>(TIMER_REG_BASE + 0x4, true, true, &m_registers[0x4], 0xFFFF, [this](u8 value, u16 offset) {
			u8 shift_amount = offset % 2;
			u16 modify = ((u16)value << (8 * shift_amount));

			u16 curr_value = m_timer_reload_val[1];

			curr_value &= ~((u16)0xFF << (8 * shift_amount));
			curr_value |= modify;

			m_timer_reload_val[1] = curr_value;
		});

		mmio->AddRegister<u16>(TIMER_REG_BASE + 0x8, true, true, &m_registers[0x8], 0xFFFF, [this](u8 value, u16 offset) {
			u8 shift_amount = offset % 2;
			u16 modify = ((u16)value << (8 * shift_amount));

			u16 curr_value = m_timer_reload_val[2];

			curr_value &= ~((u16)0xFF << (8 * shift_amount));
			curr_value |= modify;

			m_timer_reload_val[2] = curr_value;
		});

		mmio->AddRegister<u16>(TIMER_REG_BASE + 0xC, true, true, &m_registers[0xC], 0xFFFF, [this](u8 value, u16 offset) {
			u8 shift_amount = offset % 2;
			u16 modify = ((u16)value << (8 * shift_amount));

			u16 curr_value = m_timer_reload_val[3];

			curr_value &= ~((u16)0xFF << (8 * shift_amount));
			curr_value |= modify;

			m_timer_reload_val[3] = curr_value;
		});

		mmio->AddRegister<u16>(TIMER_REG_BASE + 0x2, true, true, &m_registers[0x2], 0xFFFF, 
			[this](u8 value, u16 offset) {
				if (offset % 2)
					return;

				bool enabled = (m_registers[0x2] >> 7) & 1;
				bool new_enabled_val = (value >> 7) & 1;

				if (!enabled && new_enabled_val)
					*reinterpret_cast<u16*>(m_registers + 0x0) = m_timer_reload_val[0];

				m_registers[0x2] = value;
		});

		mmio->AddRegister<u16>(TIMER_REG_BASE + 0x6, true, true, &m_registers[0x6], 0xFFFF, 
			[this](u8 value, u16 offset) {
				if (offset % 2)
					return;

				bool enabled = (m_registers[0x6] >> 7) & 1;
				bool new_enabled_val = (value >> 7) & 1;

				if (!enabled && new_enabled_val)
					*reinterpret_cast<u16*>(m_registers + 0x4) = m_timer_reload_val[1];

				m_registers[0x6] = value;
			});


		mmio->AddRegister<u16>(TIMER_REG_BASE + 0xA, true, true, &m_registers[0xA], 0xFFFF, 
			[this](u8 value, u16 offset) {
				if (offset % 2)
					return;

				bool enabled = (m_registers[0xA] >> 7) & 1;
				bool new_enabled_val = (value >> 7) & 1;

				if (!enabled && new_enabled_val)
					*reinterpret_cast<u16*>(m_registers + 0x8) = m_timer_reload_val[2];

				m_registers[0xA] = value;
			});

		mmio->AddRegister<u16>(TIMER_REG_BASE + 0xE, true, true, &m_registers[0xE], 0xFFFF, 
			[this](u8 value, u16 offset) {
				if (offset % 2)
					return;

				bool enabled = (m_registers[0xE] >> 7) & 1;
				bool new_enabled_val = (value >> 7) & 1;

				if (!enabled && new_enabled_val)
					*reinterpret_cast<u16*>(m_registers + 0xC) = m_timer_reload_val[3];

				m_registers[0xE] = value;
			});
	}

	void TimerChain::ClockCycles(u16 cycles) {
		u8 timer_cnt_index = 0x2;
		u8 timer_val_index = 0x0;

		u32 num_overflows = 0;

		for (u8 index = 0; index < 4; index++) {
			if (!CHECK_BIT(m_registers[timer_cnt_index], 7)) {
				timer_cnt_index += 0x4;
				timer_val_index += 0x4;
				num_overflows = 0;
				continue;
			}

			u32 curr_timer_val = *reinterpret_cast<u16*>(m_registers + timer_val_index);
				
			if (!CHECK_BIT(m_registers[timer_cnt_index], 2)
				|| !index) {
				u8 prescaler_sel = m_registers[timer_cnt_index] & 0x3;

				u32 prescaler = PRESCALERS[prescaler_sel];

				m_timer_internal_counter[index] += cycles;

				if (m_timer_internal_counter[index] >= prescaler) {
					curr_timer_val += m_timer_internal_counter[index] / prescaler;

					m_timer_internal_counter[index] %= prescaler;
				}
			}
			else {
				curr_timer_val += num_overflows;
			}


			if (curr_timer_val > 0xFFFF) {
				while (curr_timer_val > 0xFFFF) {
					num_overflows++;

					if (CHECK_BIT(m_registers[timer_cnt_index], 6)) {
						using memory::InterruptType;

						u16 int_id = (8 << index);

						m_int_controller->RequestInterrupt((InterruptType)int_id);
					}

					u32 timer_difference = curr_timer_val - 0x10000;

					curr_timer_val = m_timer_reload_val[index] + timer_difference;
				}
			}
			else
				num_overflows = 0;

			*reinterpret_cast<u16*>(m_registers + timer_val_index) = (u16)curr_timer_val;

			timer_cnt_index += 0x4;
			timer_val_index += 0x4;
		}
	}
}