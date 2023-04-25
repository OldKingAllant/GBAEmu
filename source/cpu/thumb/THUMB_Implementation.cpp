#include "../../../cpu/thumb/THUMB_Implementation.hpp"

#include "../../../common/Logger.hpp"
#include "../../../common/Error.hpp"

namespace GBA::cpu::thumb{
	LOG_CONTEXT(THUMB_Interpreter);

	ThumbFunc THUMB_JUMP_TABLE[20] = {};

	void ThumbUndefined(THUMBInstruction instr, memory::Bus*, CPUContext& ctx, bool&) {
		LOG_ERROR(" Undefined THUMB instruction");
		error::DebugBreak();
	}

	void InitThumbJumpTable() {
		for (u8 index = 0; index < 20; index++)
			THUMB_JUMP_TABLE[index] = ThumbUndefined;
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