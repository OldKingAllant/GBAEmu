#pragma once

#include <string_view>

#include "../common/Defs.hpp"

#include "../cpu/core/ARM7TDI.hpp"
#include "../memory/Bus.hpp"
#include "../gamepack/GamePack.hpp"

namespace GBA::emulation {
	struct EmulatorContext {
		cpu::ARM7TDI processor;
		memory::Bus bus;
		gamepack::GamePack pack;
	};

	class Emulator {
	public :
		Emulator(std::string_view rom_location);

		EmulatorContext& GetContext() {
			return m_ctx;
		}

		void RunEmulation(common::EmulatorStatus& status);

		void EmulateFor(common::u32 num_instructions);

	private :
		EmulatorContext m_ctx;
	};
}