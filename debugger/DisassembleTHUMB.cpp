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

		u32 source_reg = (instr >> 3) & 0xF;
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

	Disasm DisassembleFormat16(u16 opcode, cpu::CPUContext& ctx) {
		std::ostringstream buffer{ "" };

		u8 type = (opcode >> 8) & 0xF;

		constexpr const char* types[] = {
			"EQ", "NE", "CS", "CC", "MI", 
			"PL", "VS", "VC", "HI", "LS",
			"GE", "LT", "GT", "LE", ""
		};

		buffer << "B" << types[type] << " $";

		i16 offset = opcode & 0xFF;

		offset <<= 8;
		offset >>= 8;

		offset *= 2;
		offset += 4;

		if (offset < 0)
			buffer << "-0x" << std::hex << -offset;
		else
			buffer << "+0x" << std::hex << offset;

		return buffer.str();
	}

	Disasm DisassembleFormat18(u16 opcode, cpu::CPUContext& ctx) {
		std::ostringstream buffer{ "" };

		buffer << "B $";

		i16 offset = opcode & 0b11111111111;

		offset <<= 5;
		offset >>= 5;

		offset *= 2;
		offset += 4;

		if (offset < 0)
			buffer << "-0x" << std::hex << -offset;
		else
			buffer << "+0x" << std::hex << offset;

		return buffer.str();
	}

	Disasm DisassembleFormat1(u16 opcode, cpu::CPUContext& ctx) {
		u8 type = (opcode >> 11) & 0x3;

		u32 shift = (opcode >> 6) & 0x1F;
		u32 source = (opcode >> 3) & 0x7;
		u32 dest = opcode & 0x7;

		std::ostringstream buffer{ "" };

		constexpr const char* types[] = {
			"LSL", "LSR", "ASR", "INVALID SHIFT"
		};

		buffer << types[type] << " ";

		buffer << "r" << dest << ", ";
		buffer << "r" << source << ", ";
		buffer << "#" << shift;

		return buffer.str();
	}

	Disasm DisassembleFormat2(u16 opcode, cpu::CPUContext& ctx) {
		u8 type = (opcode >> 9) & 0x3;

		u32 reg_or_imm = (opcode >> 6) & 0x3;
		u32 source = (opcode >> 3) & 0x7;
		u32 dest = opcode & 0x7;

		std::ostringstream buffer{ "" };

		constexpr const char* types[] = {
			"ADD", "SUB", "ADD", "SUB"
		};

		buffer << types[type] << " ";

		buffer << "r" << dest << ", ";
		buffer << "r" << source << ", ";

		if (type > 0x1) {
			buffer << "# 0x" 
				<< std::hex
				<< reg_or_imm;
		}
		else
			buffer << "r" << reg_or_imm;

		return buffer.str();
	}

	Disasm DisassembleFormat4(u16 opcode, cpu::CPUContext& ctx) {
		u8 type = (opcode >> 6) & 0xF;

		u32 source_reg = (opcode >> 3) & 0x7;
		u32 dest_reg = opcode & 0x7;

		constexpr const char* types[] = {
			"AND", "EOR", "LSL", "LSR", 
			"ASR", "ADC", "SBC", "ROR",
			"TST", "NEG", "CMP", "CMN",
			"ORR", "MUL", "BIC", "MVN"
		};

		std::ostringstream buffer{ "" };

		buffer << types[type] << " ";
		buffer << "r" << dest_reg << ", ";
		buffer << "r" << source_reg;

		return buffer.str();
	}

	Disasm DisassembleFormat13(u16 opcode, cpu::CPUContext&) {
		bool type = CHECK_BIT(opcode, 7);

		u16 offset = (opcode & 0x7F) * 4;

		std::ostringstream buffer{ "" };

		if (type)
			buffer << "SUB SP, 0x" << std::hex << offset;
		else
			buffer << "ADD SP, 0x" << std::hex << offset;

		return buffer.str();
	}

	Disasm DisassembleFormat19(u16 opcode, cpu::CPUContext&) {
		u32 curr_opcode = (opcode >> 11) & 0x1F;

		std::ostringstream buffer{ "" };

		if (curr_opcode == 0x1E) {
			i32 addr_upper = opcode & 0b11111111111;

			addr_upper <<= 21;
			addr_upper >>= 21;

			buffer << "LR = $+4 + " 
				<< std::hex
				<< (addr_upper << 12);

			return buffer.str();
		}

		if (curr_opcode != 0x1F)
			return "INVALID BL";

		u32 addr_low = opcode & 0b11111111111;

		buffer << "PC = LR + " 
			<< std::hex
			<< (addr_low << 1);

		return buffer.str();
	}

	Disasm DisassembleFormat6(u16 opcode, cpu::CPUContext& ctx) {
		u32 dest_reg = (opcode >> 8) & 0x7;

		u16 offset = (opcode & 0xFF) * 4;

		std::ostringstream buffer{ "" };

		buffer << "LDR r" << dest_reg << ", [PC, # 0x";
		buffer << std::hex << offset << "]";

		return buffer.str();
	}

	Disasm DisassembleFormat7(u16 opcode, cpu::CPUContext& ctx) {
		u8 type = (opcode >> 10) & 0x3;

		u32 offset_reg = (opcode >> 6) & 0x7;
		u32 base_reg = (opcode >> 3) & 0x7;
		u32 rd = opcode & 0x7;

		constexpr const char* types[] = {
			"STR", "STRB", "LDR", "LDRB"
		};

		std::ostringstream buffer{ "" };

		buffer << types[type] << " r";
		buffer << rd << ", [";
		buffer << "r" << base_reg << ", ";
		buffer << "r" << offset_reg << "]";

		return buffer.str();
	}

	Disasm DisassembleFormat8(u16 opcode, cpu::CPUContext& ctx) {
		u8 type = (opcode >> 10) & 0x3;

		u32 offset_reg = (opcode >> 6) & 0x7;
		u32 base_reg = (opcode >> 3) & 0x7;
		u32 rd = opcode & 0x7;

		constexpr const char* types[] = {
			"STRH", "LDSB", "LDRH", "LDSH"
		};

		std::ostringstream buffer{ "" };

		buffer << types[type] << " r" << rd << ", [";
		buffer << "r" << base_reg << ", ";
		buffer << "r" << offset_reg << "]";

		return buffer.str();
	}

	Disasm DisassembleFormat9(u16 opcode, cpu::CPUContext& ctx) {
		u8 type = (opcode >> 11) & 0x3;

		u32 offset = (opcode >> 6) & 0x1F;
		u32 base_reg = (opcode >> 3) & 0x7;
		u32 rd = opcode & 0x7;

		constexpr const char* types[] = {
			"STR", "LDR", "STRB", "LDRB"
		};

		if (type < 2)
			offset *= 4;

		std::ostringstream buffer{ "" };

		buffer << types[type] << " r" << rd << ", [";
		buffer << "r" << base_reg << ", ";
		buffer << "# 0x" << std::hex << offset << "]";

		return buffer.str();
	}

	Disasm DisassembleFormat10(u16 opcode, cpu::CPUContext&) {
		bool type = CHECK_BIT(opcode, 11);

		u32 offset = ((opcode >> 6) & 0x1F) * 2;

		u32 base_reg = (opcode >> 3) & 0x7;
		u32 rd = opcode & 0x7;

		std::ostringstream buffer{ "" };

		if (type)
			buffer << "LDRH ";
		else
			buffer << "STRH ";

		buffer << "r" << rd << ", [";
		buffer << "r" << base_reg << ", ";
		buffer << "#0x" << std::hex << offset << "]";

		return buffer.str();
	}

	Disasm DisassembleFormat11(u16 opcode, cpu::CPUContext&) {
		bool type = CHECK_BIT(opcode, 11);

		u32 rd = (opcode >> 8) & 0x7;

		u32 offset = (opcode & 0xFF) * 4;

		std::ostringstream buffer{ "" };

		if (type)
			buffer << "LDR ";
		else
			buffer << "STR ";

		buffer << "r" << rd << ", [";
		buffer << "SP, #0x" << std::hex << offset << "]";

		return buffer.str();
	}

	Disasm DisassembleFormat14(u16 opcode, cpu::CPUContext&) {
		bool lr_pc = CHECK_BIT(opcode, 8);
		bool type = CHECK_BIT(opcode, 11);

		u8 rlist = opcode & 0xFF;

		std::ostringstream buffer{ "" };

		if (type) {
			buffer << "POP {";
		}
		else {
			buffer << "PUSH {";
		}

		u32 reg = 0;

		while (rlist) {
			if (rlist & 1) {
				buffer << "r" << reg;

				if (rlist >> 1)
					buffer << ", ";
			}
			
			reg++;
			rlist >>= 1;
		}

		if (lr_pc) {
			if (type)
				buffer << ", PC";
			else
				buffer << ", LR";
		}

		buffer << "}";

		return buffer.str();
	}

	Disasm DisassembleFormat15(u16 opcode, cpu::CPUContext& ctx) {
		bool type = CHECK_BIT(opcode, 11);
		u32 base_reg = (opcode >> 8) & 0x7;

		u8 rlist = opcode & 0xFF;

		std::ostringstream buffer{ "" };

		if (type)
			buffer << "LDMIA ";
		else
			buffer << "STMIA ";

		buffer << "r" << base_reg << "!, {";

		u32 reg = 0;

		while (rlist) {
			if (rlist & 1) {
				buffer << "r" << reg;

				if (rlist >> 1)
					buffer << ", ";
			}

			reg++;
			rlist >>= 1;
		}

		buffer << "}";

		return buffer.str();
	}

	Disasm DisassembleFormat17(u16 opcode, cpu::CPUContext& ctx) {
		u32 comment = opcode & 0xFF;

		std::ostringstream buffer{ "" };

		buffer << "SWI "
			<< "0x"
			<< std::hex
			<< comment;

		return buffer.str();
	}

	Disasm DisassembleTHUMB(u16 opcode, cpu::CPUContext& ctx) {
		thumb::THUMBInstructionType type = thumb::DecodeThumb(opcode);

		switch (type)
		{
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_01:
			return DisassembleFormat1(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_02:
			return DisassembleFormat2(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_03:
			return DisassembleFormat3(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_04:
			return DisassembleFormat4(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_05:
			return DisassembleFormat5(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_06:
			return DisassembleFormat6(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_07:
			return DisassembleFormat7(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_08:
			return DisassembleFormat8(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_09:
			return DisassembleFormat9(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_10:
			return DisassembleFormat10(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_11:
			return DisassembleFormat11(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_12:
			return DisassembleFormat12(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_13:
			return DisassembleFormat13(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_14:
			return DisassembleFormat14(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_15:
			return DisassembleFormat15(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_16:
			return DisassembleFormat16(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_17:
			return DisassembleFormat17(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_18:
			return DisassembleFormat18(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::FORMAT_19:
			return DisassembleFormat19(opcode, ctx);
			break;
		case GBA::cpu::thumb::THUMBInstructionType::UNDEFINED:
			break;
		default:
			break;
		}

		return "THUMB";
	}
}