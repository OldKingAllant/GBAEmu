#pragma once

#include "Instruction.hpp"

namespace GBA::cpu {
	struct CPUContext;
}

namespace GBA::memory {
	class Bus;
}

namespace GBA::cpu::arm {
	enum class ARMInstructionType {
		BRANCH,
		BRANCH_EXCHANGE,
		BLOCK_DATA_TRANSFER,
		UNDEFINED
	};

	struct ARMInstructionMask {
		static constexpr u32 BRANCH =          0b111000000000;
		static constexpr u32 BRANCH_EXCHANGE = 0b111111110000;
		static constexpr u32 BLOCK_TRANSFER =  0b111000000000;
	};

	struct ARMInstructionCode {
		static constexpr u32 BRANCH =		   0b101000000000;
		static constexpr u32 BRANCH_EXCHANGE = 0b000100100000;
		static constexpr u32 BLOCK_TRANSFER =  0b100000000000;
	};

#pragma pack(push, 1)
	union ARMBranch {
		ARMBranch(ARMInstruction instr) :
			data(instr.data) {}

		struct {
			u8  offset2;
			u16 offset;
			bool type : 1;
			u8 : 3;
			u8 condition : 4;
		};

		u32 data;
	};

	union ARMBlockTransfer {
		ARMBlockTransfer(ARMInstruction instr) :
			data(instr.data) {}

		struct {
			u16 rlist : 16;
			u8 base_reg : 4;
			bool load : 1;
			bool writeback : 1;
			bool s_bit : 1;
			bool increment : 1;
			bool pre_increment : 1;
			u8 : 3;
			u8 condition : 4;
		};

		u32 data;
	};
#pragma pack(pop)



	ARMInstructionType DecodeArm(u32 opcode);

	void ExecuteArm(ARMInstruction instr, CPUContext& ctx,  memory::Bus* bus, bool& branch);

	void DataProcessing(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch);

	//void BranchExchange()

	void Branch(ARMBranch instr, CPUContext& ctx, memory::Bus* bus, bool& branch);

	void BlockDataTransfer(ARMBlockTransfer instr, CPUContext& ctx, memory::Bus* bus, bool& branch);
}
