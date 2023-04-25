#pragma once

#include <string>
#include "../cpu/core/CPUContext.hpp"

namespace GBA::debugger {
	using namespace common;

	using Disasm = std::string;

	Disasm DisassembleTHUMB(u16 opcode, cpu::CPUContext& ctx);
}