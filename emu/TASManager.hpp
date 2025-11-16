#pragma once

#include "../common/Defs.hpp"
#include "TASParser.hpp"

#include <vector>

namespace GBA::emulation {
	class Emulator;
}

namespace GBA::tas {
	using namespace common;

	class TASManager {
	public :
		TASManager(emulation::Emulator* emu);

		void SetActions(std::vector<Action>&& acts);

		void BootComplete();

		void SetupCallback();
		void RemoveCallback();

		void EndOfFile();

		void FrameDone();

		inline bool HasActions() const {
			return !m_actions.empty();
		}

		inline size_t GetCurrentActionId() const {
			return m_curr_action;
		}

		inline std::vector<Action> const& GetActions() const {
			return m_actions;
		}

		bool DumpRemainingActions(std::string const& path) const;

	private :
		std::vector<Action> m_actions;
		size_t m_curr_action;
		u32 m_curr_action_frames;
		emulation::Emulator* m_emu;
		uint64_t m_callback_id;
		uint64_t m_curr_frame;
		uint64_t m_last_frame;
	};
}