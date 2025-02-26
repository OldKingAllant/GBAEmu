#include "BiosMath.hpp"

#include <fmt/format.h>
#include <bit>

namespace GBA::hle::math {
	static bool Div(memory::Bus* bus, cpu::CPUContext& ctx, bool& branch) {
		auto& regs = ctx.m_regs;

		auto num = int32_t(regs.GetReg(0));
		auto denom = int32_t(regs.GetReg(1));

		if (denom == 0) {
			fmt::println("[HLE] Division by zero at {:#08x}",
				regs.GetReg(15));
			return false;
		}

		auto div_signed = num / denom;
		auto mod_signed = num % denom;
		auto div_abs = std::abs(div_signed);

		regs.SetReg(0, u32(div_signed));
		regs.SetReg(1, u32(mod_signed));
		regs.SetReg(3, u32(div_abs));

		auto num_loops = std::countl_zero(u32(denom)) - std::countl_zero(u32(num));

		if (num_loops <= 0) {
			num_loops = 1;
		}

		bus->m_time.access = memory::Access::NonSeq;

		auto num_cycles = u32(40 + 13 * num_loops + 8);
		bus->InternalCycles(num_cycles);

		return true;
	}

	void RegisterMath(FunctionTable& table) {
		RegisterFunction(table, 0x06, Div);
	}
}