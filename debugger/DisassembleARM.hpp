#pragma once

#include <vector>
#include <string>

#include "../common/Defs.hpp"
#include "../cpu/core/CPUContext.hpp"

namespace GBA::debugger {
	using namespace common;

	using Disasm = std::string;

	Disasm DisassembleARM(u32 instruction, cpu::CPUContext& ctx); 
	Disasm DisassembleARMCondition(u8 code);
}