#pragma once

#include "DebugStructures.hpp"

#include <vector>

namespace GBA::emulation {
	class Emulator;
}

namespace GBA::debugger {
	class Debugger {
	public :
		Debugger(emulation::Emulator& emu);

		void Run(EmulatorStatus& status);

		std::vector<Breakpoint>& GetBreakpoints();

		void AddBreakpoint(unsigned breakpoint);
		void RemoveBreakpoint(unsigned breakpoint);
		void ToggleBreakpoint(unsigned breakpoint);

		emulation::Emulator& GetEmulator();

	private :
		void RunTillBreakpoint(EmulatorStatus& status);

		emulation::Emulator& m_emu;
		std::vector<Breakpoint> m_breakpoints;
	};
}