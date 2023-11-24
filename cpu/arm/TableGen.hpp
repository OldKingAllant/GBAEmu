#pragma once

#include "ARM_Implementation.hpp"
#include "../../common/static_for.hpp"

#include <array>

namespace GBA::cpu::arm::detail{
	//00000000 0000
	constexpr bool IsPsrTransferConstexpr(u16 opcode) {
		u8 opcode_operation = (opcode >> 5) & 0xF;

		if (opcode_operation > 0x7 && opcode_operation <= 0xB &&
			!((opcode >> 4) & 1))
			return true;

		return false;
	}

	constexpr ARMInstructionType DecodeArmConstexpr(u16 bits) {
		//First check if instruction is BX/BLX

		u32 masked = 0;

		masked = bits & ARMInstructionMask::BRANCH_EXCHANGE;

		if (masked == ARMInstructionCode::BRANCH_EXCHANGE)
			return ARMInstructionType::BRANCH_EXCHANGE;

		masked = bits & ARMInstructionMask::BRANCH;

		if (masked == ARMInstructionCode::BRANCH)
			return ARMInstructionType::BRANCH;

		masked = bits & ARMInstructionMask::BLOCK_TRANSFER;

		if (masked == ARMInstructionCode::BLOCK_TRANSFER)
			return ARMInstructionType::BLOCK_DATA_TRANSFER;

		masked = bits & ARMInstructionMask::MULTIPLY;

		if (masked == ARMInstructionCode::MULTIPLY)
			return ARMInstructionType::MULTIPLY;

		masked = bits & ARMInstructionMask::MULTIPLY_HALF;

		if (masked == ARMInstructionCode::MULTIPLY_HALF)
			return ARMInstructionType::MULTIPLY_HALF;

		masked = bits & ARMInstructionMask::ALU_IMMEDIATE;

		if (masked == ARMInstructionCode::ALU_IMMEDIATE) {
			return ARMInstructionType::DATA_PROCESSING_IMMEDIATE;
		}

		masked = bits & ARMInstructionMask::ALU_REGISTER_IMMEDIATE;

		if (masked == ARMInstructionCode::ALU_REGISTER_IMMEDIATE) {
			return ARMInstructionType::DATA_PROCESSING_REGISTER_IMM;
		}

		masked = bits & ARMInstructionMask::ALU_REGISTER_REGISTER;

		if (masked == ARMInstructionCode::ALU_REGISTER_REGISTER) {
			return ARMInstructionType::DATA_PROCESSING_REGISTER_REG;
		}

		masked = bits & ARMInstructionMask::SINGLE_HDS_TRANSFER;

		if (masked == ARMInstructionCode::SINGLE_HDS_TRANSFER) {
			if ((bits >> 1) & 0b11)
				return ARMInstructionType::SINGLE_HDS_TRANSFER;
		}

		masked = bits & ARMInstructionMask::SOFT_INTERRUPT;

		if (masked == ARMInstructionCode::SOFT_INTERRUPT)
			return ARMInstructionType::SOFT_INTERRUPT;

		masked = bits & ARMInstructionMask::SINGLE_DATA_SWAP;

		if (masked == ARMInstructionCode::SINGLE_DATA_SWAP)
			return ARMInstructionType::SINGLE_DATA_SWAP;

		masked = bits & ARMInstructionMask::SINGLE_DATA_TRANSFER_IMM;

		if (masked == ARMInstructionCode::SINGLE_DATA_TRANSFER_IMM)
			return ARMInstructionType::SINGLE_DATA_TRANSFER_IMM;

		masked = bits & ARMInstructionMask::SINGLE_DATA_TRANSFER;

		if (masked == ARMInstructionCode::SINGLE_DATA_TRANSFER)
			return ARMInstructionType::SINGLE_DATA_TRANSFER;

		return ARMInstructionType::UNDEFINED;
	}

	constexpr std::array<ARMInstructionType, 4096> GenerateArmLut() {
		std::array<ARMInstructionType, 4096> ret{};

		common::static_for<0, 4096>([&ret](u32 index, u32 dummy) {
			ARMInstructionType type = DecodeArmConstexpr(index);

			if (type == ARMInstructionType::DATA_PROCESSING_IMMEDIATE ||
				type == ARMInstructionType::DATA_PROCESSING_REGISTER_IMM ||
				type == ARMInstructionType::DATA_PROCESSING_REGISTER_REG) {
				if (IsPsrTransferConstexpr(index))
					type = ARMInstructionType::PSR_TRANSFER;
			}

			ret[index] = type;

			return dummy + 1;
		}, 0);

		return ret;
	}

	constexpr std::array<ARMInstructionType, 4096> arm_lookup_table = GenerateArmLut();

	constexpr ARMInstructionType DecodeArmFast(u32 opcode) {
		u16 hash = ((opcode >> 16) & 0xFF0) | ((opcode >> 4) & 0xF);

		return arm_lookup_table[hash];
	}
}