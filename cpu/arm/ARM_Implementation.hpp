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
		DATA_PROCESSING_IMMEDIATE,
		DATA_PROCESSING_REGISTER_REG,
		DATA_PROCESSING_REGISTER_IMM,
		PSR_TRANSFER,
		SINGLE_HDS_TRANSFER,
		SOFT_INTERRUPT,
		SINGLE_DATA_SWAP,
		MULTIPLY,
		MULTIPLY_HALF,
		SINGLE_DATA_TRANSFER_IMM,
		SINGLE_DATA_TRANSFER,
		UNDEFINED
	};

	struct ARMInstructionMask {
		static constexpr u32 BRANCH =                      0b111000000000;
		static constexpr u32 BRANCH_EXCHANGE =             0b111111111111;
		static constexpr u32 BLOCK_TRANSFER =              0b111000000000;
		static constexpr u32 ALU_REGISTER_REGISTER =	   0b111000001001;
		static constexpr u32 ALU_REGISTER_IMMEDIATE =      0b111000000001;
		static constexpr u32 ALU_IMMEDIATE =               0b111000000000;
		static constexpr u32 MULTIPLY =                    0b111100001111;
		static constexpr u32 MULTIPLY_HALF =               0b111110011001;
		static constexpr u32 SINGLE_HDS_TRANSFER =		   0b111000001001;
		static constexpr u32 SOFT_INTERRUPT =			   0b111100000000;
		static constexpr u32 SINGLE_DATA_SWAP =			   0b111110111111;
		static constexpr u32 SINGLE_DATA_TRANSFER_IMM =	   0b111000000000;
		static constexpr u32 SINGLE_DATA_TRANSFER =        0b111000000001;
	};

	struct ARMInstructionCode {
		static constexpr u32 BRANCH =		            0b101000000000;
		static constexpr u32 BRANCH_EXCHANGE =          0b000100100001;
		static constexpr u32 BLOCK_TRANSFER =           0b100000000000;
		static constexpr u32 ALU_REGISTER_REGISTER =    0b000000000001;
		static constexpr u32 ALU_REGISTER_IMMEDIATE =   0b000000000000;
		static constexpr u32 ALU_IMMEDIATE =            0b001000000000;
		static constexpr u32 MULTIPLY =                 0b000000001001;
		static constexpr u32 MULTIPLY_HALF =            0b000100001000;
		static constexpr u32 SINGLE_HDS_TRANSFER =		0b000000001001;
		static constexpr u32 SOFT_INTERRUPT =           0b111100000000;
		static constexpr u32 SINGLE_DATA_SWAP =			0b000100001001;
		static constexpr u32 SINGLE_DATA_TRANSFER_IMM = 0b010000000000;
		static constexpr u32 SINGLE_DATA_TRANSFER =     0b011000000000;
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

	union ARM_ALUImmediate {
		ARM_ALUImmediate(ARMInstruction instr) :
			data{instr.data} {}

		struct {
			u8 immediate_value : 8;
			u8 ror_shift : 4;
			u8 dest_reg : 4;
			u8 : 0;
			u8 first_op_reg : 4;
			bool s_bit : 1;
			u8 opcode_low : 3;
			u8 : 0;
			u8 opcode_hi : 1;
			bool second_operand_immediate : 1;
		};

		u32 data;
	};

	union ARM_ALURegisterReg {
		ARM_ALURegisterReg(ARMInstruction instr) :
			data{ instr.data } {}

		struct {
			u8 second_operand_reg : 4;
			bool shift_by : 1;
			u8 shift_type : 2;
			bool reserved : 1;
			u8 shift_reg : 4;
			u8 destination_reg : 4;
			u8 first_operand_reg : 4;
			bool s_bit : 1;
			u8 opcode_low : 3;
			u8 : 0;
			u8 opcode_hi : 1;
			bool second_operand_immediate : 1;
		};

		u32 data;
	};

	union ARM_ALURegisterImm {
		ARM_ALURegisterImm(ARMInstruction instr) :
			data{ instr.data } {}

		struct {
			u8 second_operand_reg : 4;
			bool shift_by : 1;
			u8 shift_type : 2;
			u8 shift_amount_lo : 1;
			u8 : 0;
			u8 shift_amount_hi : 4;
			u8 destination_reg : 4;
			u8 first_operand_reg : 4;
			bool s_bit : 1;
			u8 opcode_low : 3;
			u8 : 0;
			u8 opcode_hi : 1;
			bool second_operand_immediate : 1;
		};

		u32 data;
	};

	union ARM_SingleHDSTransfer {
		ARM_SingleHDSTransfer(ARMInstruction instr) :
			data{ instr.data } {}

		struct {
			u8 offset_low : 4;
			bool : 1;
			u8 opcode : 2;
			bool : 1;
			u8 offset_hi : 4;
			u8 source_dest_reg : 4;
			u8 base_reg : 4;
			bool load : 1;
			bool writeback : 1;
			bool immediate_offset : 1;
			bool increment : 1;
			bool pre_inc : 1;
			u8 : 3;
			u8 condition : 4;
		};

		u32 data;
	};

	union ARMPsrTransferMRS {
		ARMPsrTransferMRS(ARMInstruction instr) :
			data{ instr.data } {}

		struct {
			u8 : 8;
			u8 : 4;
			u8 dest_reg : 4;
			u8 : 0;
			u8 : 4;
			bool : 1;
			bool : 1;
			bool psr : 1;
			u8 : 1;
			u8 : 0;
			u8 : 1;
			bool immediate : 1;
		};

		u32 data;
	};

	union ARMPsrTransferMSR {
		ARMPsrTransferMSR(ARMInstruction instr) :
			data{ instr.data } {}

		struct {
			u8 : 8;
			u8 : 4;
			u8 : 4;
			u8 : 0;
			u8 flags : 4;
			bool : 1;
			bool : 1;
			bool psr : 1;
			u8 : 1;
			u8 : 1;
			bool immediate : 1;
		};

		u32 data;
	};

	union ARMBranchExchange {
		ARMBranchExchange(ARMInstruction instr) :
			data{ instr.data } {}

		struct {
			u8 operand_reg : 4;
			u8 opcode : 4;
		};

		u32 data;
	};

	union ARMMultiply {
		ARMMultiply(ARMInstruction instr) : 
			data{ instr.data } {}

		struct {
			u8 operand_reg1 : 4;
			u8 : 4;
			u8 operand_reg2 : 4;
			u8 accumul_reg : 4;
			u8 dest_reg : 4;
			bool s_bit : 1;
			u8 opcode : 3;
		};

		u32 data;
	};

	union ARMSingleDataTransfer {
		ARMSingleDataTransfer(ARMInstruction instr) :
			data{ instr.data } {}

		struct {
			u8 : 8;
			u8 : 4;
			u8 dest_reg : 4;

			u8 base_reg : 4;
			bool load : 1;
			bool writeback_or_t : 1;
			bool move_byte : 1;
			bool increment : 1;
			bool pre_incre : 1;
		};

		u32 data;
	};

	union ARMDataSwap {
		ARMDataSwap(ARMInstruction instr) :
			data{ instr.data } {}

		struct {
			u8 source_reg : 4;
			u8 : 4;
			u8 : 0;
			u8 : 4;
			u8 dest_reg : 4;
			u8 : 0;
			u8 base_reg : 4;
			u8 : 2;
			bool swap_byte : 1;
		};

		u32 data;
	};
#pragma pack(pop)

	ARMInstructionType DecodeArm(u32 opcode);

	void ExecuteArm(ARMInstruction instr, CPUContext& ctx,  memory::Bus* bus, bool& branch);
}
