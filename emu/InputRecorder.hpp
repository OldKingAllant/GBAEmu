#pragma once

#include "../common/Defs.hpp"
#include "../memory/Keypad.hpp"
#include "TASParser.hpp"

#include <vector>

namespace GBA::tas {
	class InputRecorder {
	public :
		InputRecorder();

		void DoFrame(input::Keypad const& pad);

		bool Dump(std::string const& path) const;

	private :
		std::vector<Action> m_actions;
	};
}