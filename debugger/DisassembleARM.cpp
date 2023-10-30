#include "DisassembleARM.hpp"

#include "../cpu/arm/ARM_Implementation.hpp"
#include "../common/DebugCommon.hpp"
#include "../cpu/core/CPUContext.hpp"

#include <iomanip>
#include <sstream>

#include "../common/BitManip.hpp"

#include <bit>

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

	Disasm DisassembleARMBranchExchange(arm::ARMBranchExchange instr, cpu::CPUContext& ctx) {
		std::ostringstream buffer{ "" };

		if (instr.opcode == 0x2)
			buffer << "BLX ";
		else
			buffer << "BX ";

		buffer << "r" << (u32)(instr.operand_reg);

		return buffer.str();
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

	Disasm DisassembleARMDataProcess(arm::ARMInstruction opcode, cpu::CPUContext& ctx) {
		constexpr const char* opcode_str[] = {
			"AND", "EOR", "SUB", "RSB", "ADD",
			"ADC", "SBC", "RSC", "TST", "TEQ",
			"CMP", "CMN", "ORR", "MOV", "BIC", 
			"MVN"
		};

		u8 operation = (opcode.data >> 21) & 0xF;

		std::ostringstream buffer{ "", std::ios::app };

		buffer << opcode_str[operation];

		buffer << DisassembleARMCondition(opcode.condition);

		if ((opcode.data >> 20) & 1)
			buffer << "S";

		buffer << " r";

		buffer << ((opcode.data >> 12) & 0xF);

		buffer << ", ";

		if (operation != 0x0D && operation != 0x0F) {
			u8 second_reg = ((opcode.data >> 16) & 0xF);
			u8 first_reg = ((opcode.data >> 12) & 0xF);

			if(second_reg != first_reg)
				buffer << "r" << ((opcode.data >> 16) & 0xF) << ", ";
		}
		
		if ((opcode.data >> 25) & 1) {
			u32 value = opcode.data & 0xFF;
			u8 shift = (opcode.data >> 8) & 0xF;
			shift *= 2;

			value = std::rotr(value, shift);

			buffer << "0x" << std::hex << value;

			return buffer.str();
		}

		buffer << "r" << (opcode.data & 0xF) << " ";

		constexpr const char* shift_type[] = {
			"LSL", "LSR", "ASR", "ROR"
		};

		buffer << shift_type[(opcode.data >> 5) & 0b11] << " ";

		if ((opcode.data >> 4) & 1) {
			//Shift by reg
			u8 shift_reg = (opcode.data >> 8) & 0xF;

			buffer << "r" << (u32)shift_reg;
		}
		else {
			//Shift by immediate
			u8 amount = (opcode.data >> 7) & 0x1F;

			buffer << (u32)amount;
		}

		return buffer.str();
	}

	Disasm DisassembleARMPsrTransfer(arm::ARMInstruction instr, cpu::CPUContext& ctx) {
		std::ostringstream buffer{ "" };

		if ((instr.data >> 21) & 1)
			buffer << "MSR";
		else {
			buffer << "MRS ";

			u32 dest_reg = (instr.data >> 12) & 0xF;

			buffer << "r" << dest_reg << ", ";

			if ((instr.data >> 22) & 1) {
				buffer << "SPSR";
			}
			else {
				buffer << "CPSR";
			}

			return buffer.str();
		}
		
		/*
		*  19      f  write to flags field     Bit 31-24 (aka _flg)
		   18      s  write to status field    Bit 23-16 (reserved, don't change)
		   17      x  write to extension field Bit 15-8  (reserved, don't change)
           16      c  write to control field   Bit 7-0   (aka _ctl)
		*/

		if ((instr.data >> 16) & 0xF)
			buffer << "_";

		if ((instr.data >> 19) & 1)
			buffer << "f";

		if ((instr.data >> 18) & 1)
			buffer << "s";

		if ((instr.data >> 17) & 1)
			buffer << "x";

		if ((instr.data >> 16) & 1)
			buffer << "c";

		buffer << " ";

		if ((instr.data >> 22) & 1) {
			buffer << "SPSR, ";
		}
		else {
			buffer << "CPSR, ";
		}

		if ((instr.data >> 25) & 1) {
			//Immediate
			u32 value = instr.data & 0xFF;
			u8 shift = (instr.data >> 8) & 0xF;
			shift *= 2;

			value = std::rotr(value, shift);

			buffer << "0x" << std::hex << value;
		}
		else {
			u32 source_reg = instr.data & 0xF;

			buffer << "r" << source_reg;
		}

		return buffer.str();
	}

	Disasm DisassembleARMSingleHDSTransfer(arm::ARMInstruction instr, cpu::CPUContext& ctx) {
		std::ostringstream buffer{ "" };

		std::string cond = DisassembleARMCondition(instr.condition);

		u8 load = CHECK_BIT(instr.data, 20);
		u8 opcode = (instr.data >> 5) & 3;

		constexpr const char* strings[] = {
			"", "STR", "LDR", "STR",
			"", "LDR", "LDR", "LDR"
		};

		constexpr const char* suffix[] = {
			"", "H", "D", "D",
			"", "H", "SB", "SH"
		};

		//0b110

		u8 pos = (load << 2) | opcode;

		buffer << strings[pos] << cond << suffix[pos];

		buffer << " ";

		u8 base_reg = (instr.data >> 16) & 0xF;
		u8 dest_reg = (instr.data >> 12) & 0xF;

		buffer << "r" << (u32)dest_reg;

		buffer << ", [r" << (u32)base_reg;

		if (!CHECK_BIT(instr.data, 24))
			buffer << "]";

		buffer << ", ";

		if (!CHECK_BIT(instr.data, 23))
			buffer << "-";

		if (CHECK_BIT(instr.data, 22)) {
			u32 immediate = instr.data & 0xF;
			immediate |= ((instr.data >> 8) & 0xF) << 4;

			buffer << "0x" << std::hex << immediate;
		}
		else {
			u32 reg = instr.data & 0xF;

			buffer << "r" << reg;
		}

		if (CHECK_BIT(instr.data, 24))
			buffer << "]";

		if (CHECK_BIT(instr.data, 24) &&
			CHECK_BIT(instr.data, 21)) {
			buffer << "!";
		}

		return buffer.str();
	}

	Disasm DisassembleARMSWI(arm::ARMInstruction opcode, cpu::CPUContext& ctx) {
		std::ostringstream buffer{ "" };

		buffer << "SWI";

		buffer << DisassembleARMCondition(opcode.condition);

		u32 comment = (opcode.data & 0xFFFFFF);

		buffer
			<< " 0x"
			<< std::hex
			<< comment;

		return buffer.str();
	}

	Disasm DisassembleARMDataSwap(arm::ARMInstruction opcode, cpu::CPUContext& ctx) {
		std::ostringstream buffer{ "" };

		buffer << "SWP" << DisassembleARMCondition(opcode.condition);

		if (CHECK_BIT(opcode.data, 22))
			buffer << "B";

		u32 base_reg = (opcode.data >> 16) & 0xF;
		u32 dest_reg = (opcode.data >> 12) & 0xF;
		u32 source_reg = opcode.data & 0xF;

		buffer << " "
			<< "r" << dest_reg
			<< ", r" << source_reg
			<< ", [r" << base_reg << "]";

		return buffer.str();
	}

	Disasm DisassembleARMMultiply(arm::ARMInstruction opcode, cpu::CPUContext& ctx) {
		constexpr const char* strings[] = {
			"MUL", "MLA", 
			"INVALID", "INVALID",
			"UMULL", "UMLAL",
			"SMULL", "SMLAL"
		};

		std::ostringstream buffer{ "" };

		u8 opcode_type = (opcode.data >> 21) & 0xF;

		buffer << strings[(opcode.data >> 21) & 0xF];
		buffer << DisassembleARMCondition(opcode.condition);

		if (CHECK_BIT(opcode.data, 20))
			buffer << "S";

		buffer << " ";

		u32 rd = ((opcode.data >> 16) & 0xF);
		u32 rn = ((opcode.data >> 12) & 0xF);

		u32 rs = (opcode.data >> 8) & 0xF;
		u32 rm = opcode.data & 0xF;

		switch (opcode_type)
		{
		case 0x0:
			buffer << "r" << rd << ", ";
			buffer << "r" << rm << ", ";
			buffer << "r" << rs;
			break;
		case 0x1:
			buffer << "r" << rd << ", ";
			buffer << "r" << rm << ", ";
			buffer << "r" << rs;
			buffer << ", r" << rn;
			break;
		default:
			buffer << "r" << rn << ", ";
			buffer << "r" << rd << ", ";
			buffer << "r" << rm << ", ";
			buffer << "r" << rs;
			break;
		}

		return buffer.str();
	}

	Disasm DisassembleARMMultiplyHalf(arm::ARMInstruction opcode, cpu::CPUContext& ctx) {
		return "MUL HALF";
	}

	Disasm DisassembleARMSingleDataTransfer(arm::ARMInstruction opcode, cpu::CPUContext& ctx) {
		std::ostringstream buffer{ "" };

		if (CHECK_BIT(opcode.data, 20))
			buffer << "LDR";
		else
			buffer << "STR";

		buffer << DisassembleARMCondition(opcode.condition);

		if (CHECK_BIT(opcode.data, 22))
			buffer << "B";

		if (!CHECK_BIT(opcode.data, 24) && CHECK_BIT(opcode.data, 21))
			buffer << "T";

		u32 base_reg = (opcode.data >> 16) & 0xF;
		u32 dest_reg = (opcode.data >> 12) & 0xF;

		buffer << " r" << dest_reg;

		buffer << ", [r" << (u32)base_reg;

		if (!CHECK_BIT(opcode.data, 24))
			buffer << "]";

		buffer << ", ";

		if (!CHECK_BIT(opcode.data, 23))
			buffer << "-";

		if (!CHECK_BIT(opcode.data, 25)) {
			u16 offset = opcode.data & 0xFFF;

			buffer << "0x"
				<< std::hex
				<< offset;
		}
		else {
			constexpr const char* shifts[] = {
				"LSL", "LSR", "ASR", "ROR"
			};

			u32 offset_reg = opcode.data & 0xF;

			buffer << "r" << offset_reg << ", ";

			u32 shift_amount = (opcode.data >> 7) & 0x1F;

			u8 shift_type = (opcode.data >> 5) & 0x3;

			buffer << shifts[shift_type] << " " << shift_amount;
		}

		if (CHECK_BIT(opcode.data, 24))
			buffer << "]";

		if (CHECK_BIT(opcode.data, 24) &&
			CHECK_BIT(opcode.data, 21)) {
			buffer << "!";
		}

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

		case arm::ARMInstructionType::DATA_PROCESSING_IMMEDIATE:
		case arm::ARMInstructionType::DATA_PROCESSING_REGISTER_REG:
		case arm::ARMInstructionType::DATA_PROCESSING_REGISTER_IMM:
			return DisassembleARMDataProcess(instruction, ctx);

		case arm::ARMInstructionType::PSR_TRANSFER:
			return DisassembleARMPsrTransfer(instruction, ctx);

		case arm::ARMInstructionType::SINGLE_HDS_TRANSFER:
			return DisassembleARMSingleHDSTransfer(instruction, ctx);

		case arm::ARMInstructionType::SOFT_INTERRUPT:
			return DisassembleARMSWI(instruction, ctx);

		case arm::ARMInstructionType::SINGLE_DATA_SWAP:
			return DisassembleARMDataSwap(instruction, ctx);

		case arm::ARMInstructionType::MULTIPLY:
			return DisassembleARMMultiply(instruction, ctx);

		case arm::ARMInstructionType::MULTIPLY_HALF:
			return DisassembleARMMultiplyHalf(instruction, ctx);

		case arm::ARMInstructionType::SINGLE_DATA_TRANSFER_IMM:
		case arm::ARMInstructionType::SINGLE_DATA_TRANSFER:
			return DisassembleARMSingleDataTransfer(instruction, ctx);
		}

		return "UNDEFINED";
	}
}