#include "BiosMisc.hpp"
#include "../emu/Emulator.hpp"

#include <fmt/format.h>

namespace GBA::hle::misc {

	static bool Halt(memory::Bus* bus, cpu::CPUContext& ctx, bool& branch) {
		return false;
	}

	static bool VBlankIntrWait(memory::Bus* bus, cpu::CPUContext& ctx, bool& branch) {
		return false;
	}

	void RegisterMisc(FunctionTable& ftable) {
		RegisterFunction(ftable, 0x2, Halt);
		RegisterFunction(ftable, 0x5, VBlankIntrWait);
	}

}