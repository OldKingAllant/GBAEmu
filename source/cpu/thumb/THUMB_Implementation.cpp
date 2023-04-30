#include "../../../cpu/thumb/THUMB_Implementation.hpp"

#include "../../../common/Logger.hpp"
#include "../../../common/Error.hpp"
#include "../../../common/BitManip.hpp"

namespace GBA::cpu::thumb{
	LOG_CONTEXT(THUMB_Interpreter);

	ThumbFunc THUMB_JUMP_TABLE[20] = {};

	void ThumbUndefined(THUMBInstruction instr, memory::Bus*, CPUContext& ctx, bool&) {
		LOG_ERROR(" Undefined THUMB instruction");
		error::DebugBreak();
	}

	void ThumbFormat3(THUMBInstruction instr, memory::Bus*, CPUContext& ctx, bool&) {
		u8 immediate = instr & 0xFF;
		u8 dest_reg = (instr >> 8) & 0x7;

		switch ((instr >> 11) & 0x3)
		{
		case 0x00: {
			ctx.m_regs.SetReg(dest_reg, immediate);
			ctx.m_cpsr.sign = CHECK_BIT(immediate, 7);
			ctx.m_cpsr.zero = !immediate;
		}
			break;
		case 0x01: {
			u32 reg_val = ctx.m_regs.GetReg(dest_reg);
			uint64_t res = (uint64_t)reg_val - immediate;
			//Trash the result
			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.CarrySubtract(reg_val, immediate);
			ctx.m_cpsr.OverflowSubtract(reg_val, immediate);
		}
			break;
		case 0x02 : {
			u32 reg_val = ctx.m_regs.GetReg(dest_reg);
			u32 res = reg_val + immediate;

			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.CarryAdd(reg_val, immediate);
			ctx.m_cpsr.OverflowAdd(reg_val, immediate);

			ctx.m_regs.SetReg(dest_reg, res);
		}
			break;
		case 0x03: {
			u32 reg_val = ctx.m_regs.GetReg(dest_reg);
			u32 res = reg_val - immediate;

			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.CarrySubtract(reg_val, immediate);
			ctx.m_cpsr.OverflowSubtract(reg_val, immediate);

			ctx.m_regs.SetReg(dest_reg, res);
		}
			break;
		}
	}

	void ThumbFormat5(THUMBInstruction instr, memory::Bus*, CPUContext& ctx, bool& branch) {
		u8 source_reg = (instr >> 3) & 0x7;
		u8 dest_reg = (instr & 0x7) | ((instr >> 4) &
			0b1000);

		u8 opcode = (instr >> 8) & 0x3;

		u32 source = ctx.m_regs.GetReg(source_reg);
		source += 4 * (source_reg == 15);

		switch (opcode)
		{
		case 0x00: {
			u32 first_op = ctx.m_regs.GetReg(dest_reg);

			ctx.m_regs.SetReg(dest_reg, first_op + source);
		}
			break;
		case 0x01: {
			u32 first_op = ctx.m_regs.GetReg(dest_reg);
			u32 res = first_op - source;

			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.CarrySubtract(first_op, source);
			ctx.m_cpsr.OverflowSubtract(first_op, source);
		}
			break;
		case 0x02: {
			ctx.m_regs.SetReg(dest_reg, source);
		}
			break;
		case 0x03: {
			if (!(source & 1)) {
				ctx.m_cpsr.instr_state = InstructionMode::ARM;

				if (source_reg == 15)
					source &= ~2;

				ctx.m_regs.SetReg(15, source);
			}
			else {
				ctx.m_regs.SetReg(15, source);
			}

			branch = true;
		}
			break;
		default:
			break;
		}
	}

	void ThumbFormat12(THUMBInstruction instr, memory::Bus*, CPUContext& ctx, bool& branch) {
		u32 source = 0;

		if (CHECK_BIT(instr, 11))
			source = ctx.m_regs.GetReg(13);
		else
			source = (ctx.m_regs.GetReg(15) + 4) & ~2;

		u8 dest_reg = (instr >> 8) & 0x7;
		u8 offset = (instr & 0xFF) * 4;

		ctx.m_regs.SetReg(dest_reg, source + offset);
	}

	void InitThumbJumpTable() {
		for (u8 index = 0; index < 20; index++)
			THUMB_JUMP_TABLE[index] = ThumbUndefined;

		THUMB_JUMP_TABLE[2] = ThumbFormat3;
		THUMB_JUMP_TABLE[4] = ThumbFormat5;
		THUMB_JUMP_TABLE[11] = ThumbFormat12;
	}

	THUMBInstructionType DecodeThumb(u16 opcode) {
		for (u8 i = 0; i < 19; i++) {
			u16 masked = opcode & THUMB_MASKS[i];

			if (masked == THUMB_VALUES[i])
				return (THUMBInstructionType)i;
		}

		return THUMBInstructionType::UNDEFINED;
	}

	void ExecuteThumb(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		THUMBInstructionType type = DecodeThumb(instr);

		THUMB_JUMP_TABLE[(u8)type](instr, bus, ctx, branch);
	}
}