#pragma once

#include "../cpu/core/CPUContext.hpp"
#include "../memory/Bus.hpp"

#include <array>

namespace GBA::hle {
	using namespace common;

	using FunctionHandler = bool(*)(memory::Bus* bus, cpu::CPUContext& ctx, bool& branch);
	using FunctionTable = std::array<FunctionHandler, 256>;

	void RegisterFunction(FunctionTable& table, u8 id, FunctionHandler handler);

	bool HleBiosRoutine(uint8_t id, memory::Bus* bus, cpu::CPUContext& ctx, bool& branch);
}