#include "Emulator.hpp"

#include "../common/Logger.hpp"

namespace GBA::emulation {
	LOG_CONTEXT(Emulator);

	using namespace common;

	Emulator::Emulator(std::string_view rom_location, std::optional<std::string_view> bios_location) :
		m_ctx{}, m_bios_loc{bios_location}
	{
		if (!LoadRom(rom_location))
			throw std::runtime_error("Could not load rom");
		Init();
	}

	Emulator::Emulator(std::optional<std::string_view> bios_location) :
		m_ctx{}, m_bios_loc{bios_location} {}

	bool Emulator::LoadRom(std::string_view loc) {
		return m_ctx.pack.LoadFrom(loc);
	}

	void Emulator::Init() {
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
		m_ctx.ppu.SetMMIO(m_ctx.bus.GetMMIO(), &m_ctx.bus);
		m_ctx.bus.SetPPU(&m_ctx.ppu);

		if (m_bios_loc.has_value())
			UseBIOS();
		else
			m_ctx.processor.SkipBios();

		m_ctx.keypad.SetMMIO(m_ctx.bus.GetMMIO());
		m_ctx.keypad.SetInterruptController(m_ctx.int_controller);

		m_ctx.timers.SetMMIO(m_ctx.bus.GetMMIO());
		m_ctx.timers.SetInterruptController(m_ctx.int_controller);
		m_ctx.timers.SetEventScheduler(&m_ctx.scheduler);

		m_ctx.bus.SetTimers(&m_ctx.timers);

		m_ctx.all_dma[0] = new memory::DMA(&m_ctx.bus, (u8)0, m_ctx.int_controller);
		m_ctx.all_dma[1] = new memory::DMA(&m_ctx.bus, (u8)1, m_ctx.int_controller);
		m_ctx.all_dma[2] = new memory::DMA(&m_ctx.bus, (u8)2, m_ctx.int_controller);
		m_ctx.all_dma[3] = new memory::DMA(&m_ctx.bus, (u8)3, m_ctx.int_controller);

		m_ctx.bus.SetDmas(m_ctx.all_dma);

		memory::MMIO* mmio = m_ctx.bus.GetMMIO();

		m_ctx.all_dma[0]->SetMMIO(mmio);
		m_ctx.all_dma[1]->SetMMIO(mmio);
		m_ctx.all_dma[2]->SetMMIO(mmio);
		m_ctx.all_dma[3]->SetMMIO(mmio);

		m_ctx.apu.SetDma(m_ctx.all_dma[1], m_ctx.all_dma[2]);
		m_ctx.apu.SetMMIO(mmio);
		m_ctx.apu.SetScheduler(&m_ctx.scheduler);

		m_ctx.timers.SetAPU(&m_ctx.apu);
	}

	void Emulator::EmulateFor(u32 num_instructions) {
		while (num_instructions--) {
			if(m_ctx.bus.GetActiveDma() == 4)
				m_ctx.processor.Step();
			else {
				u8 dma = m_ctx.bus.GetActiveDma();

				m_ctx.all_dma[dma]->Step();
			}

			u32 cycles = m_ctx.bus.m_time.PopCycles();
		}
	}

	void Emulator::UseBIOS() {
		m_ctx.bus.LoadBIOS(m_bios_loc.value());
		m_ctx.processor.GetContext().m_pipeline.Bubble<cpu::InstructionMode::ARM>(0x0);
	}

	void Emulator::SkipBios() {
		m_ctx.processor.SkipBios();
		m_ctx.bus.LoadBiosResetOpcode();
	}

	void Emulator::RunTillVblank() {
		while (!m_ctx.ppu.HasFrame()) {
			if (m_ctx.bus.GetActiveDma() == 4)
				m_ctx.processor.Step();
			else {
				u8 dma = m_ctx.bus.GetActiveDma();

				m_ctx.all_dma[dma]->Step();
			}

			u32 cycles = m_ctx.bus.m_time.PopCycles();
		}
	}

	Emulator::~Emulator() {
		delete m_ctx.int_controller;

		delete m_ctx.all_dma[0];
		delete m_ctx.all_dma[1];
		delete m_ctx.all_dma[2];
		delete m_ctx.all_dma[3];
	}
}