#include "Emulator.hpp"

namespace GBA::emulation {
	using namespace common;

	Emulator::Emulator(std::string_view rom_location) :
		m_ctx{}
	{
		m_ctx.pack.LoadFrom(rom_location);
		m_ctx.bus.ConnectGamepack(&m_ctx.pack);
		m_ctx.processor.AttachBus(&m_ctx.bus);
	}

	void Emulator::EmulateFor(u32 num_instructions) {
		while (num_instructions--) {
			m_ctx.processor.Step();
		}
	}

	void Emulator::RunEmulation(EmulatorStatus& status) {
		if (status.stopped)
			return;

		if (status.single_step) {
			EmulateFor(1);
			status.stopped = true;
			status.single_step = false;
			return;
		}


		status.stopped = true;
		//Do nothing
	}
}