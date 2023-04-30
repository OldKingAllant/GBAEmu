#include "DisassembleTHUMB.hpp"
#include "../cpu/thumb/THUMB_Implementation.hpp"

#include <iomanip>
#include <sstream>

#include "../common/BitManip.hpp"

namespace GBA::debugger {
	namespace thumb = cpu::thumb;

	Disasm DisassembleFormat3(u16 instr, cpu::CPUContext&) {
		constexpr const char* strings[] = {
			"MOV", "CMP", "ADD", "SUB"
		};

		std::ostringstream buffer{ "" };

		buffer << strings[(instr >> 11) & 0x3] << " ";

		buffer << "r" << ((instr >> 8) & 0b111) << ", ";

		buffer 
			<< "0x"
			<< std::hex
			<< (instr & 0xFF);

		return buffer.str();
	}

	Disasm DisassembleFormat5(u16 instr, cpu::CPUContext&) {
		constexpr const char* opcodes[] = {
			"ADD", "CMP", "MOV", "BX"
		};

		std::ostringstream buffer{ "" };

		buffer << opcodes[(instr >> 8) & 0x3] << " ";

		u32 source_reg = (instr >> 3) & 0x7;
		u32 dest_reg = (instr & 0x7) | ((instr >> 4) &
			0b1000);

		buffer << "r" << dest_reg;

		if(((instr >> 8) & 0x3) != 0x3)
			buffer << ", r" << source_reg;

		return buffer.str();
	}

	Disasm DisassembleFormat12(u16 instr, cpu::CPUContext&) {
		std::ostringstream buffer{ "" };

		buffer << "ADD ";

		u32 dest_reg = (instr >> 8) & 0x7;
		u32 offset = (instr & 0xFF) * 4;

		buffer << "r" << dest_reg << ", ";

		if (CHECK_BIT(instr, 11))
			buffer << "SP, ";
		else
			buffer << "PC, ";

		buffer << "0x"
			<< std::hex
			<< offset;

		return buffer.str();
	}

	Disasm DisassembleTHUMB(u16 opcode, cpu::CPUContext& ctx) {
		thumb::THUMBInstructionType type = thumb::DecodeThumb(opcode);

		switch (type)
		{
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_01:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_02:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_03:
			return DisassembleFormat3(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_04:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_05:
			return DisassembleFormat5(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_06:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_07:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_08:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_09:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_10:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_11:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_12:
			return DisassembleFormat12(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_13:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_14:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_15:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_16:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_17:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_18:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_19:
			break;
		case GBA::cpu::thumb::THUMBInstructionType::UNDEFINED:
			break;
		default:
			break;
		}

		return "THUMB";
	}
}