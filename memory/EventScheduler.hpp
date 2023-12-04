#pragma once

#include "../common/Defs.hpp"
#include <functional>

namespace GBA::memory {
	//We could use an std::function
	//But this prevents copy of big
	//objects
	using Callback = void(*)(void*);

	enum class EventType {
		HBLANK,
		VBLANK,
		PPUNORMAL,
		SCANLINE_INC,
		HBLANK_IN_VBLANK,
		END_VBLANK,
		TIMER_0_INC,
		TIMER_1_INC,
		TIMER_2_INC,
		TIMER_3_INC,
		APU_SAMPLE_OUT,
		APU_CH1_SEQUENCER,
		APU_CH1_SAMPLE_UPDATE,
		APU_CH2_SEQUENCER,
		APU_CH2_SAMPLE_UPDATE
	};

	struct Event {
		std::uint64_t base_timestamp;
		std::uint64_t trigger_timestamp;
		EventType type;
		Callback callback;
		void* userdata;
	};

	class EventScheduler {
	public :
		EventScheduler();

		bool Schedule(common::u32 cycles, EventType type, Callback callback, void* userdata, bool recursive = false);
		bool ScheduleAbsolute(uint64_t timestamp, EventType type, Callback callback, void* userdata, bool recursive = false);
		bool Deschedule(EventType type);
		bool Reschedule(EventType type, common::u32 cycles, bool increment_event_count);

		Event const& GetFirstEvent() const;

		uint64_t GetTimestamp() const {
			return m_timestamp;
		}

		void Advance(common::u32 cycles);

		static constexpr std::size_t MAX_EVENTS = 30;

	private :
		Event m_events[MAX_EVENTS];
		std::size_t m_num_events;
		std::uint64_t m_timestamp;
	};
}