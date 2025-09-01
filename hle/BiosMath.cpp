#include "BiosMath.hpp"

#include <fmt/format.h>
#include <bit>
#include <cmath>

namespace GBA::hle::math {
	static bool Div(memory::Bus* bus, cpu::CPUContext& ctx, bool& branch) {
		auto& regs = ctx.m_regs;

		auto num = int32_t(regs.GetReg(0));
		auto denom = int32_t(regs.GetReg(1));

		if (denom == 0) {
			fmt::print("[HLE] Division by zero at {:#010x}\n",
				regs.GetReg(15));
			return false;
		}

		if (denom == -1 &&
			num == std::numeric_limits<int32_t>::min()) {
			fmt::print("[HLE] Integer overflow at {:#010x}\n",
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
		const auto NUM_CYCLES = u32(40 + 13 * num_loops + 8);
		bus->InternalCycles(NUM_CYCLES);

		return true;
	}

	static bool Sqrt(memory::Bus* bus, cpu::CPUContext& ctx, bool& branch) {
		auto& regs = ctx.m_regs;
		auto num = regs.GetReg(0);

		//We will use directly std::sqrt, which
		//means we cannot have the correct number
		//of cycles of execution

		auto result = u16(std::sqrt(num));

		regs.SetReg(0, u32(result));

		//SWI + prologue + epilogue
		bus->m_time.access = memory::Access::NonSeq;
		const u32 NUM_CYCLES = 52;
		bus->InternalCycles(NUM_CYCLES);

		return true;
	}

	static bool ArcTan2(memory::Bus* bus, cpu::CPUContext& ctx, bool& branch) {
		auto& regs = ctx.m_regs;
		auto num_x = int32_t(regs.GetReg(0));
		auto num_y = int32_t(regs.GetReg(1));

		const u32 NUM_CYCLES_IF_ZERO = 11;
		const u32 NUM_CYCLES = 80;

		bus->m_time.access = memory::Access::NonSeq;

		regs.SetReg(3, u32(0x170));

		if (num_y == 0) {
			bus->InternalCycles(NUM_CYCLES_IF_ZERO);

			if (num_x >= 0) {
				regs.SetReg(0, 0);
			}
			else {
				regs.SetReg(0, 0x8000);
			}

			return true;
		}

		if (num_x == 0) {
			bus->InternalCycles(NUM_CYCLES_IF_ZERO);

			if (num_y >= 0) {
				regs.SetReg(0, 0x4000);
			}
			else {
				regs.SetReg(0, 0xC000);
			}

			return true;
		}

		constexpr u32 PRESCALER = 1 << 14;
		constexpr double PI = 3.14;
		constexpr auto TWO_PI = PI * 2;

		auto y_ieee = double(num_y) / PRESCALER;
		auto x_ieee = double(num_x) / PRESCALER;

		auto result_ieee = std::atan2(y_ieee, x_ieee);
		result_ieee /= TWO_PI;

		auto result = u16(result_ieee * 0xFFFF);

		regs.SetReg(0, u32(result));

		bus->InternalCycles(NUM_CYCLES);

		return true;
	}

	static bool ArcTan(memory::Bus* bus, cpu::CPUContext& ctx, bool& branch) {
		auto& regs = ctx.m_regs;
		auto tan = int32_t(regs.GetReg(0));

		const u32 NUM_CYCLES = 50;

		bus->m_time.access = memory::Access::NonSeq;

		i32 a = -((tan * tan) >> 14);
		i32 b = ((0xA9 * a) >> 14) + 0x390;

		b = ((b * a) >> 14) + 0x91C;
		b = ((b * a) >> 14) + 0xFB6;
		b = ((b * a) >> 14) + 0x16AA;
		b = ((b * a) >> 14) + 0x2081;
		b = ((b * a) >> 14) + 0x3651;
		b = ((b * a) >> 14) + 0xA2F9;

		regs.SetReg(0, u32((tan * b) >> 16));
		regs.SetReg(1, u32(a));
		regs.SetReg(3, u32(b));

		bus->InternalCycles(NUM_CYCLES);

		return true;
	}

	void RegisterMath(FunctionTable& table) {
		RegisterFunction(table, 0x06, Div);
		RegisterFunction(table, 0x08, Sqrt);
		RegisterFunction(table, 0x09, ArcTan);
		RegisterFunction(table, 0x0A, ArcTan2);
	}
}