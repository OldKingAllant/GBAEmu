#pragma once

#include <string_view>

#include "../common/Defs.hpp"

#include "../cpu/core/ARM7TDI.hpp"
#include "../memory/Bus.hpp"
#include "../gamepack/GamePack.hpp"
#include "../ppu/PPU.hpp"
#include "../memory/InterruptController.hpp"
#include "../memory/EventScheduler.hpp"
#include "../memory/Keypad.hpp"
#include "../memory/Timers.hpp"
#include "../memory/DirectMemoryAccess.hpp"
#include "../apu/APU.hpp"

namespace GBA::emulation {
	struct EmulatorContext {
		cpu::ARM7TDI processor;
		memory::Bus bus;
		gamepack::GamePack pack;
		ppu::PPU ppu;
		memory::InterruptController* int_controller;
		memory::EventScheduler scheduler;
		input::Keypad keypad;
		timers::TimerChain timers;
		memory::DMA* all_dma[4];
		apu::APU apu;
	};

	class Emulator {
	public :
		Emulator(std::string_view rom_location, std::optional<std::string_view> bios_location = std::nullopt);
		Emulator(std::optional<std::string_view> bios_location = std::nullopt);

		EmulatorContext& GetContext() {
			return m_ctx;
		}

		void EmulateFor(common::u32 num_instructions);
		void RunTillVblank();

		void UseBIOS();
		void SkipBios();

		bool LoadRom(std::string_view loc);
		void Init();

		bool StoreState(std::string const& path);
		bool LoadState(std::string const& path);

		~Emulator();

	private :
		EmulatorContext m_ctx;

		std::optional<std::string> m_bios_loc;
	};
}