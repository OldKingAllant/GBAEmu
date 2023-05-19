#include "Emulator.hpp"

namespace GBA::emulation {
	using namespace common;

	Emulator::Emulator(std::string_view rom_location, std::optional<std::string_view> bios_location) :
		m_ctx{}, m_bios_loc{std::nullopt}
	{
		m_ctx.pack.LoadFrom(rom_location);
		m_ctx.bus.ConnectGamepack(&m_ctx.pack);
		m_ctx.bus.AttachProcessor(&m_ctx.processor);

		m_ctx.int_controller = new memory::InterruptController(
			m_ctx.bus.GetMMIO()
		);

		m_ctx.processor.SetInterruptControl(m_ctx.int_controller);
		m_ctx.ppu.SetInterruptController(m_ctx.int_controller);
		m_ctx.ppu.SetScheduler(&m_ctx.scheduler);
		m_ctx.bus.SetEventScheduler(&m_ctx.scheduler);

		m_ctx.processor.AttachBus(&m_ctx.bus);
		m_ctx.ppu.SetMMIO(m_ctx.bus.GetMMIO());
		m_ctx.bus.SetPPU(&m_ctx.ppu);

		if (bios_location.has_value())
			UseBIOS(bios_location.value());
		else
			m_ctx.processor.SkipBios();

		m_ctx.keypad.SetMMIO(m_ctx.bus.GetMMIO());
		m_ctx.keypad.SetInterruptController(m_ctx.int_controller);
	}

	void Emulator::EmulateFor(u32 num_instructions) {
		while (num_instructions--) {
			m_ctx.processor.Step();

			u32 cycles = m_ctx.bus.m_time.PopCycles();

			m_ctx.ppu.ClockCycles(cycles);
		}
	}

	void Emulator::UseBIOS(std::string_view bios_loc) {
		m_bios_loc = std::string(bios_loc);

		m_ctx.bus.LoadBIOS(m_bios_loc.value());

		m_ctx.processor.GetContext().m_pipeline.Bubble<cpu::InstructionMode::ARM>(0x0);
	}

	void Emulator::SkipBios() {
		m_ctx.processor.SkipBios();
		m_ctx.bus.LoadBiosResetOpcode();
	}

	Emulator::~Emulator() {
		delete m_ctx.int_controller;
	}
}