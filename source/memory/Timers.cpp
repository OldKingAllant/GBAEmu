#include "../../memory/Timers.hpp"

#include "../../memory/MMIO.hpp"
#include "../../memory/InterruptController.hpp"
#include "../../memory/EventScheduler.hpp"

#include "../../common/BitManip.hpp"

#include "../../common/Logger.hpp"

#include "../../common/Error.hpp"

#include "../../apu/APU.hpp"

namespace GBA::timers {
	LOG_CONTEXT(Timers);

	using namespace common;

	TimerChain::TimerChain() :
		m_int_controller(nullptr), m_sched(nullptr), 
		m_registers{}, m_timer_reload_val{},
		m_timer_internal_counter{}, m_apu(nullptr),
		m_last_read_timestamp{},
		m_last_event_timestamp{}
	{}

	void TimerChain::TimerOverfow(common::u8 timer_id) {
		u16 cnt_pos = 0x2 + 4 * timer_id;
		u16 val_pos = 4 * timer_id;

		u8 control = m_registers[cnt_pos];

		if (CHECK_BIT(control, 6)) {
			using memory::InterruptType;

			u16 int_id = (u16)8 << timer_id;

			m_int_controller->RequestInterrupt((InterruptType)int_id);
		}

		u32 curr_timer_val = m_timer_reload_val[timer_id];

		*reinterpret_cast<u16*>(m_registers + val_pos) = curr_timer_val;

		if (timer_id == 3)
			return;

		u16 cnt_pos2 = cnt_pos + 0x4;
		u16 val_pos2 = val_pos + 0x4;

		if (!CHECK_BIT(m_registers[cnt_pos2], 2))
			return;

		//Next Timer is count up
		u32 next_val = *reinterpret_cast<u16*>(m_registers + val_pos2);

		next_val++;

		if (next_val > 0xFFFF) {
			TimerOverfow(timer_id + 1);
		}
		else {
			*reinterpret_cast<u16*>(m_registers + val_pos2) = next_val;
		}
	}

	/*
	TODO: Rename this, since it is called when
	an overflow happens (not an increment)
	*/

	template <u8 Id>
	void TimerIncremented(void* _timers);

	template <>
	void TimerIncremented<0>(void* _timers) {
		TimerChain* timers = std::bit_cast<TimerChain*>(_timers);
		timers->TimerOverfow(0);
		timers->m_apu->TimerOverflow(0);

		u8 prescaler = timers->m_registers[0x2] & 3;

		u32 time_till_ov = ((u32)0x10000 - timers->m_timer_reload_val[0]) * TimerChain::PRESCALERS[prescaler];

		uint64_t now = timers->m_last_event_timestamp[0];

		timers->m_last_read_timestamp[0] = now;
		timers->m_last_event_timestamp[0] = now + time_till_ov;

		timers->m_sched->ScheduleAbsolute(
			timers->m_last_event_timestamp[0], memory::EventType::TIMER_0_INC,
			TimerIncremented<0>, _timers, true
		);
	}

	template <>
	void TimerIncremented<1>(void* _timers) {
		TimerChain* timers = std::bit_cast<TimerChain*>(_timers);
		timers->TimerOverfow(1);
		timers->m_apu->TimerOverflow(1);

		u8 prescaler = timers->m_registers[0x6] & 3;

		u32 time_till_ov = ((u32)0x10000 - timers->m_timer_reload_val[1]) * TimerChain::PRESCALERS[prescaler];

		uint64_t now = timers->m_last_event_timestamp[1];

		timers->m_last_read_timestamp[1] = now;
		timers->m_last_event_timestamp[1] = now + time_till_ov;

		timers->m_sched->ScheduleAbsolute(
			timers->m_last_event_timestamp[1], memory::EventType::TIMER_1_INC,
			TimerIncremented<1>, _timers, true
		);
	}

	template <>
	void TimerIncremented<2>(void* _timers) {
		TimerChain* timers = std::bit_cast<TimerChain*>(_timers);
		timers->TimerOverfow(2);

		u8 prescaler = timers->m_registers[0xA] & 3;

		u32 time_till_ov = ((u32)0x10000 - timers->m_timer_reload_val[2]) * TimerChain::PRESCALERS[prescaler];

		uint64_t now = timers->m_last_event_timestamp[2];

		timers->m_last_read_timestamp[2] = now;
		timers->m_last_event_timestamp[2] = now + time_till_ov;

		timers->m_sched->ScheduleAbsolute(
			timers->m_last_event_timestamp[2], memory::EventType::TIMER_2_INC,
			TimerIncremented<2>, _timers, true
		);
	}

	template <>
	void TimerIncremented<3>(void* _timers) {
		TimerChain* timers = std::bit_cast<TimerChain*>(_timers);
		timers->TimerOverfow(3);

		u8 prescaler = timers->m_registers[0xE] & 3;

		u32 time_till_ov = ((u32)0x10000 - timers->m_timer_reload_val[3]) * TimerChain::PRESCALERS[prescaler];

		uint64_t now = timers->m_last_event_timestamp[3];

		timers->m_last_read_timestamp[3] = now;
		timers->m_last_event_timestamp[3] = now + time_till_ov;

		timers->m_sched->ScheduleAbsolute(
			timers->m_last_event_timestamp[3], memory::EventType::TIMER_3_INC,
			TimerIncremented<3>, _timers, true
		);
	}

	void TimerChain::SetInterruptController(memory::InterruptController* int_control) {
		m_int_controller = int_control;
	}

	void TimerChain::SetEventScheduler(memory::EventScheduler* ev_sched) {
		m_sched = ev_sched;

		m_sched->SetEventTypeRodata(memory::EventType::TIMER_0_INC,
			TimerIncremented<0>, std::bit_cast<void*>(this));
		m_sched->SetEventTypeRodata(memory::EventType::TIMER_1_INC,
			TimerIncremented<1>, std::bit_cast<void*>(this));
		m_sched->SetEventTypeRodata(memory::EventType::TIMER_2_INC,
			TimerIncremented<2>, std::bit_cast<void*>(this));
		m_sched->SetEventTypeRodata(memory::EventType::TIMER_3_INC,
			TimerIncremented<3>, std::bit_cast<void*>(this));
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

				RecalculateEvents(0, value);
		});

		mmio->AddRegister<u16>(TIMER_REG_BASE + 0x6, true, true, &m_registers[0x6], 0xFFFF, 
			[this](u8 value, u16 offset) {
				if (offset % 2)
					return;

				RecalculateEvents(1, value);
			});


		mmio->AddRegister<u16>(TIMER_REG_BASE + 0xA, true, true, &m_registers[0xA], 0xFFFF, 
			[this](u8 value, u16 offset) {
				if (offset % 2)
					return;

				RecalculateEvents(2, value);
			});

		mmio->AddRegister<u16>(TIMER_REG_BASE + 0xE, true, true, &m_registers[0xE], 0xFFFF, 
			[this](u8 value, u16 offset) {
				if (offset % 2)
					return;

				RecalculateEvents(3, value);
			});
	}

	void TimerChain::RecalculateEvents(u8 timer_id, u8 new_cnt) {
		u16 cnt_pos = 0x2 + 4 * timer_id;
		u16 val_pos = 4 * timer_id;

		if (new_cnt == m_registers[cnt_pos])
			return;

		bool enabled = (m_registers[cnt_pos] >> 7) & 1;
		bool new_enabled_val = (new_cnt >> 7) & 1;
		bool count_up = (new_cnt >> 2) & 1;

		m_registers[cnt_pos] = new_cnt;

		memory::EventType timer_event = memory::EventType::TIMER_0_INC;

		void(*callback)(void*) = nullptr;

		switch (timer_id)
		{
		case 0:
			m_sched->Deschedule(memory::EventType::TIMER_0_INC);
			timer_event = memory::EventType::TIMER_0_INC;
			callback = TimerIncremented<0>;
			break;
		case 1:
			m_sched->Deschedule(memory::EventType::TIMER_1_INC);
			timer_event = memory::EventType::TIMER_1_INC;
			callback = TimerIncremented<1>;
			break;
		case 2:
			m_sched->Deschedule(memory::EventType::TIMER_2_INC);
			timer_event = memory::EventType::TIMER_2_INC;
			callback = TimerIncremented<2>;
			break;
		case 3:
			m_sched->Deschedule(memory::EventType::TIMER_3_INC);
			timer_event = memory::EventType::TIMER_3_INC;
			callback = TimerIncremented<3>;
			break;
		default:
			error::Unreachable();
			break;
		}

		if (!new_enabled_val || count_up)
			return;

		if (!enabled && new_enabled_val)
			*reinterpret_cast<u16*>(m_registers + val_pos) = m_timer_reload_val[timer_id];

		u8 prescaler = new_cnt & 3;

		u32 time_till_ov = ((u32)0x10000 - m_timer_reload_val[timer_id]) * PRESCALERS[prescaler];
		
		m_last_read_timestamp[timer_id] = m_sched->GetTimestamp();
		m_last_event_timestamp[timer_id] = m_last_read_timestamp[timer_id] + time_till_ov;

		m_sched->Schedule(time_till_ov, timer_event, callback,
			std::bit_cast<void*>(this));
	}

	void TimerChain::SetAPU(apu::APU* apu) {
		m_apu = apu;
	}

	void TimerChain::Update() {
		u8 timer_cnt_index = 0x2;
		u8 timer_val_index = 0x0;

		uint64_t now = m_sched->GetTimestamp();

		for (u8 index = 0; index < 4; index++) {
			if (!CHECK_BIT(m_registers[timer_cnt_index], 7) || 
				CHECK_BIT(m_registers[timer_cnt_index], 2)) {
				timer_cnt_index += 0x4;
				timer_val_index += 0x4;
				continue;
			}

			u32 curr_val = *reinterpret_cast<u16*>(m_registers + timer_val_index);
			u32 orig_val = curr_val;
			u8 prescaler = m_registers[timer_cnt_index] & 0x3;

			curr_val += ((now - m_last_read_timestamp[index]) / PRESCALERS[prescaler]);
			curr_val &= 0xFFFF;

			*reinterpret_cast<u16*>(m_registers + timer_val_index) = curr_val;

			//Modify timestamp only if enough cycles have passed, and
			//the timer has been incremented
			if(curr_val != orig_val)
				m_last_read_timestamp[index] = now;

			timer_cnt_index += 0x4;
			timer_val_index += 0x4;
		}
	}
}