#include "DisassembleARM.hpp"

#include "../cpu/arm/ARM_Implementation.h"
#include "../common/DebugCommon.hpp"
#include "../cpu/core/CPUContext.hpp"

#include <iomanip>
#include <sstream>

namespace GBA::debugger {
	namespace arm = cpu::arm;

	template <typename T>
	std::string to_hex(T const& value) {
		std::ostringstream stream = {};

		stream << "0x"
			<< std::setfill('0')
			<< std::setw(sizeof(T) * 2) << std::hex
			<< value;

		return stream.str();
	}

	Disasm DisassembleARMCondition(u8 code) {
		/*
		Code Suffix Flags         Meaning
  0:   EQ     Z=1           equal (zero) (same)
  1:   NE     Z=0           not equal (nonzero) (not same)
  2:   CS/HS  C=1           unsigned higher or same (carry set)
  3:   CC/LO  C=0           unsigned lower (carry cleared)
  4:   MI     N=1           negative (minus)
  5:   PL     N=0           positive or zero (plus)
  6:   VS     V=1           overflow (V set)
  7:   VC     V=0           no overflow (V cleared)
  8:   HI     C=1 and Z=0   unsigned higher
  9:   LS     C=0 or Z=1    unsigned lower or same
  A:   GE     N=V           greater or equal
  B:   LT     N<>V          less than
  C:   GT     Z=0 and N=V   greater than
  D:   LE     Z=1 or N<>V   less or equal
  E:   AL     -             always (the "AL" suffix can be omitted)
  F:   NV     -             never (ARMv1,v2 only) (Reserved ARMv3 and up)
  */
		constexpr const char* text[] = {
			"EQ", "NE", "CS", "CC", "MI",
			"PL", "VS", "VC", "HI", "LS",
			"GE", "LT", "GT", "LE", "", 
			"NV"
		};

		return text[code];
	}

	Disasm DisassembleARMBranch(arm::ARMBranch branch, cpu::CPUContext& ctx) {
		std::ostringstream buffer{ "B", std::ios::app };

		if (branch.type)
			buffer << "L";

		buffer << DisassembleARMCondition(branch.condition);

		buffer << " ";

		i32 offset = (branch.offset << 8) | branch.offset2;
		offset <<= 8;
		offset >>= 8;
		offset *= 4;

		buffer << to_hex(ctx.m_regs.GetReg(15) + offset + 8);

		return buffer.str();
	}

	Disasm DisassembleARMBranchExchange(arm::ARMInstruction, cpu::CPUContext& ctx) {
		return "";
	}

	Disasm DisassembleARMBlockTransfer(arm::ARMBlockTransfer transfer, cpu::CPUContext& ctx) {
		//STM/LDM{cond}{amod} Rn{!},<Rlist>{^}
		std::ostringstream buffer{ "", std::ios::app };

		if (transfer.load)
			buffer << "LDM";
		else
			buffer << "STM";

		buffer << DisassembleARMCondition(transfer.condition);

		constexpr const char* type[] = {
			//STM
			"ED", "EA", 
			"FD", "FA",
			//LDM
			"FA", "FD",
			"EA", "ED"
		};

		u8 type_id = !!transfer.load << 2 | !!transfer.pre_increment << 1 | !!transfer.increment;

		buffer << type[type_id];

		buffer << " r" << IntToString(transfer.base_reg) << (transfer.writeback ?
			"!" : "") << ", {";

		u8 index = 0;

		while (transfer.rlist) {
			if (transfer.rlist & 1) {
				buffer << "r" << IntToString(index);

				if (transfer.rlist >> 1)
					buffer << ", ";
			}

			transfer.rlist >>= 1;

			++index;
		}

		buffer << "}";

		if (transfer.s_bit)
			buffer << "^";

		return buffer.str();
	}

	Disasm DisassembleARM(u32 opcode, cpu::CPUContext& ctx) {
		arm::ARMInstructionType type = arm::DecodeArm(opcode);

		arm::ARMInstruction instruction{ opcode };

		switch (type) {
		case arm::ARMInstructionType::BRANCH:
			return DisassembleARMBranch(instruction, ctx);

		case arm::ARMInstructionType::BRANCH_EXCHANGE:
			return DisassembleARMBranchExchange(instruction, ctx);

		case arm::ARMInstructionType::BLOCK_DATA_TRANSFER:
			return DisassembleARMBlockTransfer(instruction, ctx);
		}

		return "UNDEFINED";
	}
}