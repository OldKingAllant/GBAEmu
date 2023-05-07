#include "Emulator.hpp"

namespace GBA::emulation {
	using namespace common;

	Emulator::Emulator(std::string_view rom_location) :
		m_ctx{}
	{
		m_ctx.pack.LoadFrom(rom_location);
		m_ctx.bus.ConnectGamepack(&m_ctx.pack);
		m_ctx.bus.AttachProcessor(&m_ctx.processor);
		m_ctx.processor.AttachBus(&m_ctx.bus);
		m_ctx.ppu.SetMMIO(m_ctx.bus.GetMMIO());
		m_ctx.bus.SetPPU(&m_ctx.ppu);
	}

	void Emulator::EmulateFor(u32 num_instructions) {
		while (num_instructions--) {
			m_ctx.processor.Step();

			u32 cycles = m_ctx.bus.m_time.PopCycles();

			m_ctx.ppu.ClockCycles(cycles);
		}
	}
}