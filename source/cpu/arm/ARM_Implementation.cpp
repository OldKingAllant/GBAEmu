#include "../../../cpu/arm/ARM_Implementation.h"
#include "../../../cpu/core/CPUContext.hpp"
#include "../../../memory/Bus.hpp"

#include <bit>

#include "../../../common/Logger.hpp"
#include "../../../common/Error.hpp"
#include "../../../common/BitManip.hpp"

namespace GBA::cpu::arm{
	LOG_CONTEXT(ARM_Interpreter);

	namespace detail {
		void AND(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void EOR(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void SUB(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void RSB(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void ADD(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void ADC(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void SBC(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void RSC(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void TST(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void TEQ(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void CMP(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void CMN(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}

		void ORR(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 first_reg = ctx.m_regs.GetReg(first_op);

			u32 res = first_reg | value;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !!res;
				ctx.m_cpsr.sign = CHECK_BIT(value, 31);
			}
		}

		void MOV(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			(void)first_op;

			ctx.m_regs.SetReg(dest, value);

			if (s_bit) {
				ctx.m_cpsr.zero = !!value;
				ctx.m_cpsr.sign = CHECK_BIT(value, 31);
			}
		}

		void BIC(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}
		void MVN(u8 dest, u8 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			const char* err = __FUNCTION__ " Not implemented";
			LOG_ERROR(err);
			error::DebugBreak();
		}

		void DataProcessingCommon(u8 dest, u8 first_op, u8 opcode, u32 value, bool s_bit, 
			CPUContext& ctx, bool& branch) {
			bool old_s_bit = s_bit;

			if (dest == 15 && s_bit)
				s_bit = false;

			switch (opcode)
			{
			case 0x00:
				AND(dest, first_op, value, s_bit, ctx);
				break;
			case 0x01:
				EOR(dest, first_op, value, s_bit, ctx);
				break;
			case 0x02:
				SUB(dest, first_op, value, s_bit, ctx);
				break;
			case 0x03:
				RSB(dest, first_op, value, s_bit, ctx);
				break;
			case 0x04:
				ADD(dest, first_op, value, s_bit, ctx);
				break;
			case 0x05:
				ADC(dest, first_op, value, s_bit, ctx);
				break;
			case 0x06:
				SBC(dest, first_op, value, s_bit, ctx);
				break;
			case 0x07:
				RSC(dest, first_op, value, s_bit, ctx);
				break;
			case 0x08:
				TST(dest, first_op, value, s_bit, ctx);
				break;
			case 0x09:
				TEQ(dest, first_op, value, s_bit, ctx);
				break;
			case 0x0A:
				CMP(dest, first_op, value, s_bit, ctx);
				break;
			case 0x0B:
				CMN(dest, first_op, value, s_bit, ctx);
				break;
			case 0x0C:
				ORR(dest, first_op, value, s_bit, ctx);
				break;
			case 0x0D:
				MOV(dest, first_op, value, s_bit, ctx);
				break;
			case 0x0E:
				BIC(dest, first_op, value, s_bit, ctx);
				break;
			case 0x0F:
				MVN(dest, first_op, value, s_bit, ctx);
				break;
			default:
				break;
			}

			if (dest != 15)
				return;

			if (old_s_bit) {
				if (ctx.m_cpsr.mode == Mode::User ||
					ctx.m_cpsr.mode == Mode::SYS) {
					LOG_ERROR("S Bit used in USER/SYS mode");
					error::DebugBreak();
				}

				ctx.RestorePreviousMode(ctx.m_regs.GetReg(15));
			}

			branch = true;
		}
	}

	bool IsPsrTransfer(u32 opcode) {
		u8 opcode_operation = (opcode >> 21) & 0xF;

		if (opcode_operation >= 0x7 && opcode_operation <= 0xB &&
			!((opcode >> 20) & 1))
			return true;

		return false;
	}

	ARMInstructionType DecodeArm(u32 opcode) {
		//Get bits [27,20] and bits [7,4]
		u16 bits = (((opcode >> 20) & 0xFF) << 4) | ((opcode >> 4) & 0xF);
		u32 masked = bits & ARMInstructionMask::BRANCH;

		if (masked == ARMInstructionCode::BRANCH)
			return ARMInstructionType::BRANCH;

		masked = bits & ARMInstructionMask::BLOCK_TRANSFER;

		if (masked == ARMInstructionCode::BLOCK_TRANSFER)
			return ARMInstructionType::BLOCK_DATA_TRANSFER;

		masked = bits & ARMInstructionMask::MULTIPLY;

		if (masked == ARMInstructionCode::MULTIPLY)
			return ARMInstructionType::UNDEFINED;

		masked = bits & ARMInstructionMask::MULTIPLY_HALF;

		if (masked == ARMInstructionCode::MULTIPLY_HALF)
			return ARMInstructionType::UNDEFINED;

		masked = bits & ARMInstructionMask::ALU_IMMEDIATE;

		if (masked == ARMInstructionCode::ALU_IMMEDIATE) {
			if (IsPsrTransfer(opcode))
				return ARMInstructionType::PSR_TRANSFER;

			return ARMInstructionType::DATA_PROCESSING_IMMEDIATE;
		}
		
		masked = bits & ARMInstructionMask::ALU_REGISTER_IMMEDIATE;

		if (masked == ARMInstructionCode::ALU_REGISTER_IMMEDIATE) {
			if (IsPsrTransfer(opcode))
				return ARMInstructionType::PSR_TRANSFER;

			return ARMInstructionType::DATA_PROCESSING_REGISTER_IMM;
		}

		masked = bits & ARMInstructionMask::ALU_REGISTER_REGISTER;

		if (masked == ARMInstructionCode::ALU_REGISTER_REGISTER) {
			if (IsPsrTransfer(opcode))
				return ARMInstructionType::PSR_TRANSFER;

			return ARMInstructionType::DATA_PROCESSING_REGISTER_REG;
		}

		masked = bits & ARMInstructionMask::SINGLE_HDS_TRANSFER;

		if (masked == ARMInstructionCode::SINGLE_HDS_TRANSFER) {
			if ((opcode >> 5) & 0b11)
				return ARMInstructionType::SINGLE_HDS_TRANSFER;
		}

		return ARMInstructionType::UNDEFINED;
	}

	void Branch(ARMBranch instr, CPUContext& ctx,  memory::Bus* bus, bool& branch) {
		branch = true;

		if (instr.type) {
			//BL
			ctx.m_regs.SetReg(14, ctx.m_regs.GetReg(15) + 4);
		}

		i32 offset = (instr.offset << 8) | instr.offset2;
		offset <<= 8;
		offset >>= 8;
		offset *= 4;

		ctx.m_regs.AddOffset(15, offset + 8);
	}

	void inline StoreBlock(ARMBlockTransfer instr, CPUContext& ctx,  memory::Bus* bus, bool& branch) {
		u16 list = instr.rlist;

		u8 reg_id = 0;

		u8 base_reg = instr.base_reg;

		u32 base = ctx.m_regs.GetReg(base_reg);

		if (list == 0) {
			//Empty reglist
			//The PC is saved
			//The other register are not stored
			//but the base register is modified
			//with an offset of 0x40
			bus->Write<u32>(base, ctx.m_regs.GetReg(15));

			instr.writeback = true;
			
			if (instr.increment)
				ctx.m_regs.AddOffset(base_reg, 0x40);
			else
				ctx.m_regs.AddOffset(base_reg, -64);

			return;
		}

		u8 pre_increment = 0;
		u8 post_increment = 0;

		if (instr.pre_increment)
			pre_increment = 4;
		else
			post_increment = 4;

		if (instr.s_bit) {
			if (instr.writeback) {
				//Writeback with S bit set is not allowed
				LOG_ERROR("Writeback with S bit is not allowed");
				error::DebugBreak();
			}

			while (list) {
				if (list & 1) {
					base += pre_increment;

					bus->Write<u32>(base, ctx.m_regs.GetReg(Mode::User, reg_id));

					base += post_increment;
				}

				list >>= 1;
				reg_id++;
			}
		}
		else {
			while (list) {
				if (list & 1) {
					base += pre_increment;

					bus->Write<u32>(base, ctx.m_regs.GetReg(reg_id));

					base += post_increment;
				}

				list >>= 1;
				reg_id++;
			}
		}

		ctx.m_regs.SetReg(base_reg, base);
	}

	void inline LoadBlock(ARMBlockTransfer instr, CPUContext& ctx,  memory::Bus* bus, bool& branch) {
		LOG_ERROR("LDM Not implemented");
		error::DebugBreak();
	}

	void BlockDataTransfer(ARMBlockTransfer instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		//The register list is 16 bits long, and 
		//when a bit is 1 the corresponding register 
		//r[n] is in the list

		//Get base register
		u32 base = ctx.m_regs.GetReg(instr.base_reg);
		u32 original_base = base;

		if (instr.s_bit && (ctx.m_cpsr.mode == Mode::User ||
			ctx.m_cpsr.mode == Mode::SYS)) {
			return;
		}

		if (!instr.increment) {
			//Registers are processed in increasing
			//addresses, which means that if we
			//are decrementing the address after
			//each load/store, we should start
			//from the lowest address and then increment

			u8 popcnt = std::popcount(instr.rlist);

			base -= popcnt * 4;
		}

		ctx.m_regs.SetReg(instr.base_reg, base);

		if (instr.load) {
			LoadBlock(instr, ctx, bus, branch);
			//Load from memory
		}
		else {
			StoreBlock(instr, ctx, bus, branch);
			//Write to memory
		}

		if ((instr.rlist >> instr.base_reg) & 1) {
			//Rb is in reg list
			
			if (instr.load)
				instr.writeback = false;
			else if(instr.writeback) {
				//If it is the first in the list
				//the value stays unchanged
				if (!(instr.rlist & ~(1 << instr.base_reg))) {
					instr.writeback = false;
				}

				//Else, behaviour is not
				//specified.
			}
		}
		
		if (!instr.writeback)
			ctx.m_regs.SetReg(instr.base_reg, original_base);
		else if (!instr.increment && instr.rlist)
			ctx.m_regs.SetReg(instr.base_reg, base);
	}

	template <typename InstructionT>
	void DataProcessing(InstructionT instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {}

	

	template <>
	void DataProcessing(ARM_ALUImmediate instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		u8 dest_reg = instr.dest_reg;
		u8 first_op_reg = instr.first_op_reg;

		u8 real_opcode = (instr.opcode_hi << 3) | instr.opcode_low;

		if (first_op_reg && (real_opcode == 0xD || real_opcode == 0xF)) {
			LOG_ERROR("First operand register is != 0 with MOV/MVN");
			error::DebugBreak();
		}

		u32 value = instr.immediate_value;
		u8 shift = instr.ror_shift * 2;

		value = std::rotr(value, shift);

		detail::DataProcessingCommon(dest_reg, first_op_reg, real_opcode, value, instr.s_bit, ctx, branch);
	}

	template <>
	void DataProcessing(ARM_ALURegisterReg instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {

	}

	template <>
	void DataProcessing(ARM_ALURegisterImm instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {

	}

	void PsrTransfer(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		LOG_ERROR("PSR Transfer Not implemented");
		error::DebugBreak();
	}

	void SingleHDSTransfer(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		LOG_ERROR("Single HDS Transfer Not implemented");
		error::DebugBreak();
	}

	void ExecuteArm(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		if (!ctx.m_cpsr.CheckCondition(instr.condition))
			return;

		ARMInstructionType type = DecodeArm(instr.data);

		switch (type)
		{
		case ARMInstructionType::BRANCH:
			Branch(instr, ctx, bus, branch);
			break;
		case ARMInstructionType::BRANCH_EXCHANGE:
			break;
		case ARMInstructionType::BLOCK_DATA_TRANSFER:
			BlockDataTransfer(instr, ctx, bus, branch);
			break;
		case ARMInstructionType::DATA_PROCESSING_IMMEDIATE:
			DataProcessing(ARM_ALUImmediate{ instr }, ctx, bus, branch);
			break;
		case ARMInstructionType::DATA_PROCESSING_REGISTER_REG:
			DataProcessing(ARM_ALURegisterReg{ instr }, ctx, bus, branch);
			break;
		case ARMInstructionType::DATA_PROCESSING_REGISTER_IMM:
			DataProcessing(ARM_ALURegisterImm{ instr }, ctx, bus, branch);
			break;
		case ARMInstructionType::PSR_TRANSFER:
			PsrTransfer(instr, ctx, bus, branch);
			break;
		case ARMInstructionType::SINGLE_HDS_TRANSFER:
			SingleHDSTransfer(instr, ctx, bus, branch);
			break;
		case ARMInstructionType::UNDEFINED:
			break;
		default:
			break;
		}
	}
}