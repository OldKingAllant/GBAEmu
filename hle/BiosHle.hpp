#pragma once

#include "../cpu/core/CPUContext.hpp"
#include "../memory/Bus.hpp"

namespace GBA::hle {
	bool HleBiosRoutine(uint8_t id, memory::Bus* bus, cpu::CPUContext& ctx, bool& branch);
}