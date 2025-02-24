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
		m_rewind_pos{}, m_enable_rewind{false},
		
		m_reset_state{}, m_is_init{false}, 
		
		m_cheats{}, m_enabled_cheats{},
		m_hooks{}, m_enable_hooks{false}
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

		auto& db_cheats = m_ctx.pack.GetCheats();

		for (auto& [cheat_name, cheat_entry] : db_cheats) {
			AddCheat(
				cheat_entry.lines,
				cheat_entry.ty,
				cheat_name
			);
		}
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
		if (m_enable_hooks) {
			ProcessCheats();
		}

		while (!m_ctx.ppu.HasFrame()) {
			if (m_ctx.bus.GetActiveDma() == 4) {
				m_ctx.processor.Step();

				if (m_enable_hooks && !m_hooks.empty()) {
					auto curr_pc = m_ctx.processor.GetContext()
						.m_regs.GetReg(15);
					auto hooks = m_hooks.equal_range(curr_pc);

					for (; hooks.first != hooks.second; hooks.first++) {
						auto const& hook = hooks.first->second;
						auto& cheat_set = m_cheats[hook];
						cheats::RunCheatInterpreter(cheat_set, this);
					}
				}
			}
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

	void Emulator::SetRewindEnable(bool enable_rewind) {
		if (!enable_rewind && m_enable_rewind) {
			m_rewind_pos = 0;
			m_rewind_buf.clear();
		}
		m_enable_rewind = enable_rewind;
	}

	bool Emulator::RewindPush() {
		std::string temp_buf{};
		savestate::StoreToBuffer(temp_buf, this);
		
		m_rewind_buf.push_front(std::move(temp_buf));

		if (m_rewind_buf.size() > m_rewind_buf_size) {
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

	bool Emulator::AddCheat(std::vector<std::string> lines, cheats::CheatType ty, std::string name) {
		if (m_cheats.find(name) != m_cheats.cend()) { return false; }

		auto cheat_set = cheats::ParseCheat(lines, ty);

		if (!cheat_set.directives.empty()) {
			m_cheats[name] = cheat_set;
		}

		return !cheat_set.directives.empty();
	}

	void Emulator::RemoveCheat(std::string const& name) {
		auto position = m_cheats.find(name);
		if (position == m_cheats.cend()) { return; }

		DisableCheat(name);
		m_cheats.erase(position);
	}

	bool Emulator::EnableCheat(std::string const& name) {
		if (m_cheats.find(name) == m_cheats.cend()) { return false; }
		if (std::find(m_enabled_cheats.cbegin(), m_enabled_cheats.cend(), name)
			!= m_enabled_cheats.cend()) 
		{ return false; }

		auto& cheat_set = m_cheats[name];
		cheat_set.enabled = true;

		if (!cheats::ApplyCheatPatches(name, cheat_set, this))
			return false;

		m_enabled_cheats.push_back(name);
		return true;
	}

	void Emulator::DisableCheat(std::string const& name) {
		auto pos = std::find(m_enabled_cheats.begin(),
			m_enabled_cheats.end(),
			name);

		if (pos == m_enabled_cheats.end())
			return;
		
		m_enabled_cheats.erase(pos);

		auto& cheat_set = m_cheats[name];
		cheat_set.enabled = false;

		cheats::RestoreCheatPatches(name, cheat_set, this);
	}

	void Emulator::ProcessCheats() {
		for (auto const& cheat_name : m_enabled_cheats) {
			auto& cheat_set = m_cheats[cheat_name];
			if (!cheat_set.contains_hook) {
				cheats::RunCheatInterpreter(cheat_set, this);
			}
		}
	}

	void Emulator::AddHook(uint32_t pc, std::string const& name) {
		auto range = m_hooks.equal_range(pc);

		if (std::find_if(
			range.first, range.second,
			[&name](auto const& hook) {
				return hook.second == name;
			}
		) != range.second)
			return;

		m_hooks.insert({ pc, name });
	}

	void Emulator::RemoveHook(std::string const& name) {
		auto position = std::find_if(
			m_hooks.begin(), m_hooks.end(),
			[&name](auto const& hook) {
				return hook.second == name;
			}
		);

		if (position != m_hooks.end())
			m_hooks.erase(position);
	}

	Emulator::~Emulator() {
		delete m_ctx.int_controller;

		delete m_ctx.all_dma[0];
		delete m_ctx.all_dma[1];
		delete m_ctx.all_dma[2];
		delete m_ctx.all_dma[3];
	}
}