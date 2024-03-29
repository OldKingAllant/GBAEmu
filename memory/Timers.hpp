#pragma once

#include "../common/Defs.hpp"

namespace GBA::memory {
	class MMIO;
	class InterruptController;
	class EventScheduler;
}

namespace GBA::apu {
	class APU;
}

namespace GBA::timers {
	class TimerChain {
	public :
		TimerChain();

		void SetMMIO(memory::MMIO* mmio);
		void SetInterruptController(memory::InterruptController* int_control);
		void SetEventScheduler(memory::EventScheduler* ev_sched);
		void SetAPU(apu::APU* apu);

		void Update();

		static constexpr common::u32 TIMER_REG_BASE = 0x100;
		static constexpr common::u32 CPU_FREQ = 16'780'000;

		static constexpr common::u32 PRESCALERS[] = {
			1, 64, 256, 1024
		};

	private :
		memory::InterruptController* m_int_controller;
		memory::EventScheduler* m_sched;

		common::u8 m_registers[0x10];

		common::u16 m_timer_reload_val[4];
		common::u32 m_timer_internal_counter[4];

		apu::APU* m_apu;

		uint64_t m_last_read_timestamp[4];
		uint64_t m_last_event_timestamp[4];

		void RecalculateEvents(common::u8 timer_id, common::u8 new_cnt);

		template <common::u8 Id>
		friend void TimerIncremented(void* _timers);

		void TimerOverfow(common::u8 timer_id);
	};
}