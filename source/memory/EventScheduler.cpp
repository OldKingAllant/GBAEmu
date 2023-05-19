#include "../../memory/EventScheduler.hpp"

#include <algorithm>

namespace GBA::memory {
	using namespace common;

	EventScheduler::EventScheduler() :
		m_events{}, m_num_events{0},
		m_timestamp{0}
	{}

	bool EventScheduler::Schedule(u32 cycles, EventType type, Callback callback, void* userdata, bool recursive) {
		uint64_t next_timestamp = m_timestamp + cycles;

		return ScheduleAbsolute(next_timestamp, type, callback, userdata, recursive);
		
		/*if (!recursive && std::find_if(m_events, m_events + m_num_events,
			[type](Event const& ev) {
				return ev.type == type;
			}
		) != m_events + m_num_events)
			return false;

		if (m_num_events == MAX_EVENTS)
			return false;

		uint64_t next_timestamp = m_timestamp + cycles;

		m_events[m_num_events++] = Event{
			m_timestamp, next_timestamp, type, callback, userdata
		};

		std::push_heap(m_events, m_events + m_num_events, 
			[](Event const& ev1, Event const& ev2) {
				return ev1.trigger_timestamp > ev2.trigger_timestamp;
			}
		);

		return true;*/
	}

	bool EventScheduler::ScheduleAbsolute(uint64_t timestamp, EventType type, Callback callback, void* userdata, bool recursive) {
		if (!recursive && std::find_if(m_events, m_events + m_num_events,
			[type](Event const& ev) {
				return ev.type == type;
			}
		) != m_events + m_num_events)
			return false;

		if (m_num_events == MAX_EVENTS)
			return false;

		uint64_t next_timestamp = timestamp;

		m_events[m_num_events++] = Event{
			m_timestamp, next_timestamp, type, callback, userdata
		};

		std::push_heap(m_events, m_events + m_num_events,
			[](Event const& ev1, Event const& ev2) {
				return ev1.trigger_timestamp > ev2.trigger_timestamp;
			}
		);

		return true;
	}

	bool EventScheduler::Deschedule(EventType type) {
		Event* ev = std::find_if(m_events, m_events + m_num_events,
			[type](Event const& ev) {
				return ev.type == type;
			});

		if (ev == m_events + m_num_events)
			return false;

		std::swap(*ev, m_events[m_num_events - 1]);

		m_num_events--;

		std::make_heap(m_events, m_events + m_num_events,
			[](Event const& ev1, Event const& ev2) {
				return ev1.trigger_timestamp > ev2.trigger_timestamp;
			}
		);

		return true;
	}

	bool EventScheduler::Reschedule(EventType type, u32 cycles, bool increment_event_count) {
		Event* ev = std::find_if(m_events, m_events + m_num_events,
			[type](Event const& ev) {
				return ev.type == type;
			});

		if (ev == m_events + m_num_events)
			return false;

		ev->trigger_timestamp = m_timestamp + cycles;

		std::make_heap(m_events, m_events + m_num_events,
			[](Event const& ev1, Event const& ev2) {
				return ev1.trigger_timestamp > ev2.trigger_timestamp;
			}
		);

		if (increment_event_count)
			m_num_events++;

		return true;
	}

	Event const& EventScheduler::GetFirstEvent() const {
		return m_events[0];
	}

	void EventScheduler::Advance(common::u32 cycles) {
		m_timestamp += cycles;

		bool _cont = true;

		do {
			if (!m_num_events) {
				_cont = false;
				continue;
			}

			Event const& ev = m_events[0];

			if (ev.trigger_timestamp > m_timestamp) {
				_cont = false;
			}
			else {
				if(ev.callback)
					ev.callback(ev.userdata);

				std::pop_heap(m_events, m_events + m_num_events,
					[](Event const& ev1, Event const& ev2) {
						return ev1.trigger_timestamp > ev2.trigger_timestamp;
					}
				);

				m_num_events--;
			}
		} while (_cont);
	}
}