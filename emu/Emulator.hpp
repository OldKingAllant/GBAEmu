#pragma once

#include <string_view>

#include "../common/Defs.hpp"

#include "../cpu/core/ARM7TDI.hpp"
#include "../memory/Bus.hpp"
#include "../gamepack/GamePack.hpp"
#include "../ppu/PPU.hpp"
#include "../memory/InterruptController.hpp"

namespace GBA::emulation {
	struct EmulatorContext {
		cpu::ARM7TDI processor;
		memory::Bus bus;
		gamepack::GamePack pack;
		ppu::PPU ppu;
		memory::InterruptController* int_controller;
	};

	class Emulator {
	public :
		Emulator(std::string_view rom_location, std::optional<std::string_view> bios_location = std::nullopt);

		EmulatorContext& GetContext() {
			return m_ctx;
		}

		void EmulateFor(common::u32 num_instructions);

		void UseBIOS(std::string_view bios_loc);
		void SkipBios();

		~Emulator();

	private :
		EmulatorContext m_ctx;

		std::optional<std::string> m_bios_loc;
	};
}