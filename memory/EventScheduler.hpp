#pragma once

#include "../common/Defs.hpp"

#include <functional>
#include <array>

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
		APU_CH2_SAMPLE_UPDATE,
		APU_CH3_SEQUENCER,
		APU_CH3_SAMPLE_UPDATE,
		APU_CH4_SEQUENCER,
		APU_CH4_SAMPLE_UPDATE,
		EVENT_MAX
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

		void SetEventTypeRodata(EventType ev_ty, Callback callback, void* data);

		bool Schedule(common::u32 cycles, EventType type, Callback callback, void* userdata, bool recursive = false);
		bool ScheduleAbsolute(uint64_t timestamp, EventType type, Callback callback, void* userdata, bool recursive = false);
		bool Deschedule(EventType type);
		bool Reschedule(EventType type, common::u32 cycles, bool increment_event_count);

		Event const& GetFirstEvent() const;

		inline uint64_t GetTimestamp() const {
			return m_timestamp;
		}

		void Advance(common::u32 cycles);

		static constexpr std::size_t MAX_EVENTS = 30;

		template <typename Ar>
		void save(Ar& ar) const {
			using namespace common;

			ar(m_num_events);
			ar(m_timestamp);

			for (u32 curr_event = 0; curr_event < m_num_events; curr_event++) {
				ar(m_events[curr_event].base_timestamp);
				ar(m_events[curr_event].trigger_timestamp);
				ar(m_events[curr_event].type);
			}
		}

		template <typename Ar>
		void load(Ar& ar) {
			using namespace common;

			uint64_t old_ev_count{};

			ar(old_ev_count);
			ar(m_timestamp);

			m_num_events = 0;

			for (u32 curr_event = 0; curr_event < old_ev_count; curr_event++) {
				uint64_t base{}, trigger{};
				EventType ty{};

				ar(base);
				ar(trigger);
				ar(ty);

				auto callback = m_event_type_rodata[u32(ty)].first;
				auto data = m_event_type_rodata[u32(ty)].second;

				ScheduleAbsolute(trigger, ty, callback, data);
			}
		}

		using EvTypeData = std::pair<Callback, void*>;

	private :
		std::array<EvTypeData, size_t(EventType::EVENT_MAX)> m_event_type_rodata;
		Event m_events[MAX_EVENTS];
		std::size_t m_num_events;
		std::uint64_t m_timestamp;
	};
}