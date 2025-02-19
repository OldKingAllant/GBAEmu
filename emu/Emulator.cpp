#include "Emulator.hpp"

#include "../common/Logger.hpp"
#include "SaveState.hpp"

#include <fstream>
#include <filesystem>

namespace GBA::emulation {
	LOG_CONTEXT(Emulator);

	using namespace common;

	Emulator::Emulator() :
		m_ctx{}, m_bios_loc{},
		m_rewind_buf_size{}, m_rewind_buf{},
		m_rewind_pos{}, m_reset_state{},
		m_is_init{false}
	{}

	Emulator::Emulator(std::string_view rom_location, std::string_view bios_location) :
		Emulator()
	{
		m_bios_loc = bios_location;
		if (!LoadRom(rom_location))
			throw std::runtime_error("Could not load rom");
		Init();
	}

	Emulator::Emulator(std::string_view bios_location) :
		Emulator()
	{
		m_bios_loc = bios_location;
	}


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

		UseBIOS();

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

	void Emulator::SaveResetState() {
		savestate::StoreToBuffer(m_reset_state, this);
		m_is_init = true;
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
		m_ctx.bus.LoadBIOS(m_bios_loc);
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

	void Emulator::StoreState(std::string const& path) {
		std::ofstream out{ path, std::ios::out };

		if (!out.is_open()) {
			fmt::println("Save state failed!");
			return;
		}

		out.close();
		out.open(path, std::ios::out | std::ios::binary);

		savestate::StoreToFile(out, this);
	}

	void Emulator::LoadState(std::string const& path) {
		std::ifstream in{ path, std::ios::in | std::ios::binary };

		if (!in.is_open()) {
			fmt::println("Load state failed!");
			return;
		}

		savestate::LoadFromFile(in, this);
	}

	bool Emulator::RewindPush() {
		std::string temp_buf{};
		savestate::StoreToBuffer(temp_buf, this);
		
		m_rewind_buf.push_front(std::move(temp_buf));

		if (m_rewind_buf.size() >= m_rewind_buf_size) {
			m_rewind_buf.pop_back();
		}

		return true;
	}

	bool Emulator::LoadFromCurrentHistoryPosition() {
		auto state = m_rewind_buf.begin();

		for (u32 curr_pos = 1; curr_pos < m_rewind_pos; curr_pos++) {
			state++;
		}

		savestate::LoadFromBuffer(*state, this);
		return true;
	}

	bool Emulator::RewindBackward() {
		if (m_rewind_buf.empty() || m_rewind_pos == m_rewind_buf.size())
			return false;

		m_rewind_pos++;
		return LoadFromCurrentHistoryPosition();
	}

	bool Emulator::RewindForward() {
		if (m_rewind_buf.empty() || m_rewind_pos == 0)
			return false;

		m_rewind_pos--;
		return LoadFromCurrentHistoryPosition();
	}

	bool Emulator::RewindPop() {
		if (m_rewind_buf.empty() || m_rewind_pos == 0)
			return false;

		while (m_rewind_pos > 0) {
			m_rewind_buf.pop_front();
			m_rewind_pos--;
		}

		return true;
	}

	bool Emulator::Reset() {
		if (!m_is_init)
			return false;

		savestate::LoadFromBuffer(m_reset_state, this);
		m_rewind_buf.clear();
		m_rewind_pos = 0;
		return true;
	}

	Emulator::~Emulator() {
		delete m_ctx.int_controller;

		delete m_ctx.all_dma[0];
		delete m_ctx.all_dma[1];
		delete m_ctx.all_dma[2];
		delete m_ctx.all_dma[3];
	}
}