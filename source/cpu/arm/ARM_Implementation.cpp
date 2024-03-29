#include "../../../cpu/arm/ARM_Implementation.hpp"
#include "../../../cpu/core/CPUContext.hpp"
#include "../../../memory/Bus.hpp"

#include <bit>

#include "../../../common/Logger.hpp"
#include "../../../common/Error.hpp"
#include "../../../common/BitManip.hpp"

#include "../../../cpu/arm/TableGen.hpp"

#include <utility>

namespace GBA::cpu::arm{
	LOG_CONTEXT(ARM_Interpreter);

	namespace detail {
		void AND(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = first_op & value;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			}
		}

		void EOR(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = first_op ^ value;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			}
		}

		void SUB(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = first_op - value;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
				ctx.m_cpsr.CarrySubtract(first_op, value);
				ctx.m_cpsr.OverflowSubtract(first_op, value);
			}
		}

		void RSB(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = value - first_op;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
				ctx.m_cpsr.CarrySubtract(value, first_op);
				ctx.m_cpsr.OverflowSubtract(value, first_op);
			}
		}

		void ADD(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = first_op + value;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
				ctx.m_cpsr.CarryAdd(first_op, value);
				ctx.m_cpsr.OverflowAdd(first_op, value);
			}
		}

		void ADC(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u8 carry_val = ctx.m_cpsr.carry;

			uint64_t res = (uint64_t)first_op + value + carry_val;
			u32 res32 = (u32)res;

			ctx.m_regs.SetReg(dest, res32);

			if (s_bit) {
				ctx.m_cpsr.zero = !res32;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
				ctx.m_cpsr.carry = res >> 32;
				ctx.m_cpsr.overflow = (~(first_op ^ value) & (value ^ (u32)res32)) >> 31;
			}
		}

		void SBC(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u8 carry_val = ctx.m_cpsr.carry ^ 1;

			u32 res = first_op - value - carry_val;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
				ctx.m_cpsr.carry = (uint64_t)first_op >= (uint64_t)value + (uint64_t)carry_val;
				ctx.m_cpsr.overflow = ((first_op ^ value) & (first_op ^ res)) >> 31;
			}
		}

		void RSC(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u8 carry_val = ctx.m_cpsr.carry ^ 1;

			u32 res = value - first_op - carry_val;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
				ctx.m_cpsr.carry = (uint64_t)value >= (uint64_t)first_op + (uint64_t)carry_val;
				ctx.m_cpsr.overflow = ((value ^ first_op) & (value ^ res)) >> 31;
			}
		}

		void TST(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = first_op & value;

			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
		}

		void TEQ(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = first_op ^ value;

			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
		}

		void CMP(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = first_op - value;

			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			ctx.m_cpsr.CarrySubtract(first_op, value);
			ctx.m_cpsr.OverflowSubtract(first_op, value);
		}

		void CMN(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = first_op + value;

			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			ctx.m_cpsr.CarryAdd(first_op, value);
			ctx.m_cpsr.OverflowAdd(first_op, value);
		}

		void ORR(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = first_op | value;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			}
		}

		void MOV(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			(void)first_op;

			ctx.m_regs.SetReg(dest, value);

			if (s_bit) {
				ctx.m_cpsr.zero = !value;
				ctx.m_cpsr.sign = CHECK_BIT(value, 31);
			}
		}

		void BIC(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u32 res = first_op & ~value;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			}
		}

		void MVN(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			value = ~value;

			ctx.m_regs.SetReg(dest, value);

			if (s_bit) {
				ctx.m_cpsr.zero = !value;
				ctx.m_cpsr.sign = CHECK_BIT(value, 31);
			}
		}

		template <u8 Opcode, bool S>
		void DataProcessingCommon(u8 dest, u32 first_op, u8 opcode, u32 value, bool s_bit,
			CPUContext& ctx, bool& branch, bool shift_carry) {
			if (dest == 15 && s_bit)
				s_bit = false;

			if constexpr (Opcode == 0) {
				AND(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
			}
			else if constexpr (Opcode == 1) {
				EOR(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
			}
			else if constexpr (Opcode == 2) {
				SUB(dest, first_op, value, s_bit, ctx);
			}
			else if constexpr (Opcode == 3) {
				RSB(dest, first_op, value, s_bit, ctx);
			}
			else if constexpr (Opcode == 4) {
				ADD(dest, first_op, value, s_bit, ctx);
			}
			else if constexpr (Opcode == 5) {
				ADC(dest, first_op, value, s_bit, ctx);
			}
			else if constexpr (Opcode == 6) {
				SBC(dest, first_op, value, s_bit, ctx);
			}
			else if constexpr (Opcode == 7) {
				RSC(dest, first_op, value, s_bit, ctx);
			}
			else if constexpr (Opcode == 8) {
				TST(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
			}
			else if constexpr (Opcode == 9) {
				TEQ(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
			}
			else if constexpr (Opcode == 0xA) {
				CMP(dest, first_op, value, s_bit, ctx);
			}
			else if constexpr (Opcode == 0xB) {
				CMN(dest, first_op, value, s_bit, ctx);
			}
			else if constexpr (Opcode == 0xC) {
				ORR(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
			}
			else if constexpr (Opcode == 0xD) {
				MOV(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
			}
			else if constexpr (Opcode == 0xE) {
				BIC(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
			}
			else if constexpr (Opcode == 0xF) {
				MVN(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
			}

			if (dest != 15)
				return;

			if constexpr (S) {
				if (ctx.m_cpsr.mode != Mode::User &&
					ctx.m_cpsr.mode != Mode::SYS) {
					ctx.RestorePreviousMode(ctx.m_regs.GetReg(15));
				}
			}

			if constexpr (Opcode <= 0x7 || Opcode > 0xB)
				branch = true;
			//Else, bad CMP/CMN... instruction
		}

		template <bool Load, u8 Opcode>
		inline u32 HDSTransfer(i32 base, u8 reg, CPUContext& ctx, memory::Bus* bus, bool& branch) {
			if constexpr (!Load) {
				if constexpr (Opcode == 0x1) {
					u32 to_write = ctx.m_regs.GetReg(reg);
					to_write += 12 * (reg == 15);

					bus->m_time.access = Access::NonSeq;

					bus->Write<u16>(base, to_write);
				}
				else if constexpr (Opcode == 2) {
					LOG_ERROR("LDRD not implemented");
					error::DebugBreak();
				}
				else if constexpr (Opcode == 3) {
					LOG_ERROR("STRD not implemented");
					error::DebugBreak();
				}
			}
			else {
				if constexpr (Opcode == 1) {
					bus->m_time.access = Access::NonSeq;

					u32 value = bus->Read<u16>(base);

					value = std::rotr(value, 8 * (base & 1));

					ctx.m_regs.SetReg(reg, value);

					bus->InternalCycles(1);
				}
				else if constexpr (Opcode == 2) {
					bus->m_time.access = Access::NonSeq;

					int32_t value = (int8_t)bus->Read<u8>(base);
					ctx.m_regs.SetReg(reg, value);

					bus->InternalCycles(1);
				}
				else if constexpr (Opcode == 3) {
					int32_t value = 0;

					bus->m_time.access = Access::NonSeq;

					if (base & 1)
						value = (int8_t)bus->Read<u8>(base);
					else
						value = (int16_t)bus->Read<u16>(base);

					ctx.m_regs.SetReg(reg, value);

					bus->InternalCycles(1);
				}
			}

			return base;
		}

		template <bool Imm>
		std::pair<u32, bool> LSL(u32 value, u8 amount, u8 old_carry) {
			if (!amount)
				return { value, old_carry };

			u8 bit_pos = (32 - amount);

			u32 res = 0;

			if (amount == 32)
				return { 0, value & 1 };

			if (amount >= 32)
				return { 0, false };
			
			res = value << amount;

			return { res, CHECK_BIT(value, bit_pos) };
		}

		template <bool Imm>
		std::pair<u32, bool> LSR(u32 value, u8 amount, u8 old_carry) {
			if constexpr (Imm) {
				if (!amount)
					amount = 32;
			}
			else {
				if (!amount)
					return { value, old_carry };
			}
			
			u8 bit_pos = (amount - 1) * !!amount;

			u32 res = 0;

			if (amount < 32) {
				res = value >> amount;
			}
			
			return { res, CHECK_BIT(value, bit_pos) * (amount < 33) };
		}

		template <bool Imm>
		std::pair<u32, bool> ASR(u32 value, u8 amount, u8 old_carry) {
			if constexpr (!Imm) {
				if (!amount)
					return { value, old_carry };
			}
			else {
				if (!amount || amount >= 32)
					amount = 32;
			}

			u8 bit_pos = (amount - 1);
			u32 sign = CHECK_BIT(value, 31);

			i32 res = 0;

			if constexpr (Imm) {
				if (amount < 32)
					res = (i32)value >> amount;
				else {
					for (int8_t pos = 31; pos >= (int8_t)(32 - amount); pos--)
						res |= (sign << pos);
				}
			}
			else {
				if (amount >= 32) {
					amount = 31;
					bit_pos = 31;
				}
					

				res = (i32)value >> amount;
			}

			return { (u32)res, CHECK_BIT(value, bit_pos) };
		}

		template <bool Imm>
		std::pair<u32, bool> ROR(u32 value, u8 amount, u8 old_carry) {
			if constexpr (!Imm) {
				if (!amount)
					return { value, old_carry };
			}


			u8 bit_pos = (amount - 1) % 32;

			u32 res = 0;

			if (amount > 32)
				amount %= 32;

			if (!amount) {
				res = value >> 1;
				res |= (old_carry << 31);
				bit_pos = 0;
			}
			else 
				res = std::rotr(value, amount);

			return { res, CHECK_BIT(value, bit_pos) };
		}

		template <bool Imm, bool Spsr>
		void MrsTransfer(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool&) {
			static_assert(sizeof(ARMPsrTransferMRS) == 4);

			ARMPsrTransferMRS instr = instr_orig;

			bus->m_time.access = Access::Seq;

			if constexpr (Spsr) {
				if (ctx.m_cpsr.mode == Mode::User ||
					ctx.m_cpsr.mode == Mode::SYS) [[unlikely]] {
				
						ctx.m_regs.SetReg(instr.dest_reg,
					ctx.m_cpsr);
						return;
				}

				ctx.m_regs.SetReg(instr.dest_reg,
					ctx.m_spsr[GetModeFromID(ctx.m_cpsr.mode) - 1]);
			}
			else
				ctx.m_regs.SetReg(instr.dest_reg,
					ctx.m_cpsr);
		}

		template <bool Imm, bool Spsr>
		void MsrTransfer(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool&) {
			static_assert(sizeof(ARMPsrTransferMSR) == 4);

			ARMPsrTransferMSR instr = instr_orig;

			bus->m_time.access = Access::Seq;

			u32 value = 0;

			if constexpr (Imm) {
				value = instr.data & 0xFF;
				u8 shift = (instr.data >> 8) & 0xF;
				shift *= 2;

				value = std::rotr(value, shift);
			}
			else
				value = ctx.m_regs.GetReg(instr.data & 0xF);

			u32 mask = 0;

			u8 flags = instr.flags;
			u8 pos = 0;

			while (flags) {
				mask |= ((flags & 1) * 0xFF) << pos;
				flags >>= 1;
				pos += 8;
			}

			value &= mask;

			if constexpr (Spsr) {
				if (ctx.m_cpsr.mode == Mode::User ||
					ctx.m_cpsr.mode == Mode::SYS)
					return;

				u8 mode_id = GetModeFromID(ctx.m_cpsr.mode);
				u32 psr = ctx.m_spsr[mode_id - 1];
				psr &= ~mask;
				psr |= value;
				ctx.m_spsr[mode_id - 1] = psr;
			}
			else {
				u32 psr = ctx.m_cpsr;
				psr &= ~mask;
				psr |= value;

				CPSR new_cpsr{};
				new_cpsr = psr;

				new_cpsr.instr_state = ctx.m_cpsr.instr_state;

				if (ctx.m_cpsr.mode != new_cpsr.mode)
					ctx.ChangeMode(new_cpsr.mode);

				ctx.m_cpsr = new_cpsr;
			}
		}
	}

	bool IsPsrTransfer(u32 opcode) {
		u8 opcode_operation = (opcode >> 21) & 0xF;

		if (opcode_operation > 0x7 && opcode_operation <= 0xB &&
			!((opcode >> 20) & 1))
			return true;

		return false;
	}

	ARMInstructionType DecodeArm(u32 opcode) {
		//First check if instruction is BX/BLX

		u32 masked = 0;
		
		constexpr const u32 BRANCH_EXCHANGE_MASK =
			0b00001111111111111111111111000000;

		constexpr const u32 BRANCH_EXCHANGE_VALUE =
			0b00000001001011111111111100000000;

		masked = opcode & BRANCH_EXCHANGE_MASK;

		if (masked == BRANCH_EXCHANGE_VALUE)
			return ARMInstructionType::BRANCH_EXCHANGE;

		//Get bits [27,20] and bits [7,4]
		u16 bits = (((opcode >> 20) & 0xFF) << 4) | ((opcode >> 4) & 0xF);
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

	template <bool Bl>
	void Branch(ARMInstruction instr_orig, CPUContext& ctx,  memory::Bus* bus, bool& branch) {
		static_assert(sizeof(ARMBranch) == 4);

		ARMBranch instr = instr_orig;

		/*
		* This takes 2S + 1N waitstates:
		* 1S -> Prefetch
		* 1N + 1S -> Pipeline flush
		* 
		* Next access type set by pipeline
		*/
		branch = true;

		if constexpr (Bl) {
			//BL
			ctx.m_regs.SetReg(14, ctx.m_regs.GetReg(15) + 4);
		}

		i32 offset = (instr.offset << 8) | instr.offset2;
		offset <<= 8;
		offset >>= 8;
		offset *= 4;

		ctx.m_regs.AddOffset(15, offset + 8);

		bus->m_time.access = Access::Seq;
	}

	template <bool PreInc, bool AddOffset, bool Psr, bool Writeback>
	void inline StoreBlock(ARMBlockTransfer instr, CPUContext& ctx,  memory::Bus* bus, bool& branch, u32 base, u32 new_base) {
		static_assert(sizeof(ARMBlockTransfer) == 4);

		u16 list = instr.rlist;

		u8 reg_id = 0;

		u8 base_reg = instr.base_reg;

		bus->m_time.access = Access::NonSeq;

		if (list == 0) {
			//Empty reglist
			//The PC is saved
			//The other register are not stored
			//but the base register is modified
			//with an offset of 0x40
			i8 offset = 0;

			if constexpr (!AddOffset) {
				if constexpr (PreInc)
					offset = -0x3C;
				else
					offset = -0x40;
			}
			else {
				if constexpr (!PreInc)
					offset = 0x0;
				else
					offset = 0x4;
			}

			bus->Write<u32>((i32)base + offset, ctx.m_regs.GetReg(15) + 12);

			instr.writeback = true;
			
			if constexpr (AddOffset)
				base += 0x40;
			else
				base -= 0x40;

			ctx.m_regs.SetReg(base_reg, base);

			return;
		}

		u8 pre_increment = PreInc ? 4 : 0;
		u8 post_increment = !PreInc ? 4 : 0;

		u32 reg_value = 0;

		u8 zeroes = std::countr_zero(list);

		list >>= zeroes;
		reg_id += zeroes;

		u8 first_reg_id = reg_id;

		if constexpr (Psr) {
			if constexpr (Writeback) {
				//Writeback with S bit set is not allowed
				LOG_ERROR("Writeback with S bit is not allowed");
				error::DebugBreak();
			}

			while (list) {
				if (list & 1) {
					reg_value = ctx.m_regs.GetReg(Mode::User, reg_id) + 12 * (reg_id == 15);

					base += pre_increment;

					bus->Write<u32>(base, reg_value);

					base += post_increment;

					bus->m_time.access = Access::Seq;
				}

				list >>= 1;
				reg_id++;
			}
		}
		else {
			while (list) {
				if (list & 1) {
					if (reg_id == base_reg && instr.writeback) {
						if (reg_id == first_reg_id)
							reg_value = ctx.m_regs.GetReg(base_reg);
						else
							reg_value = new_base;
					}
					else 
						reg_value = ctx.m_regs.GetReg(reg_id) + 12 * (reg_id == 15);

					base += pre_increment;

					bus->Write<u32>(base, reg_value);

					base += post_increment;

					bus->m_time.access = Access::Seq;
				}

				list >>= 1;
				reg_id++;
			}
		}

		bus->m_time.access = Access::NonSeq;
	}


	template <bool PreInc, bool AddOffset, bool Psr, bool Writeback>
	void inline LoadBlock(ARMBlockTransfer instr, CPUContext& ctx,  memory::Bus* bus, bool& branch, u32 base) {
		u16 list = instr.rlist;

		u8 reg_id = 0;

		u8 base_reg = instr.base_reg;

		bus->m_time.access = Access::NonSeq;

		if (list == 0) {
			i8 offset = 0;

			if constexpr (!AddOffset) {
				if constexpr (PreInc)
					offset = -0x3C;
				else
					offset = -0x40;
			}
			else {
				if constexpr (!PreInc)
					offset = 0x0;
				else
					offset = 0x4;
			}

			u32 value = bus->Read<u32>((i32)base + offset);

			ctx.m_regs.SetReg(15, value);

			instr.writeback = true;

			if constexpr (AddOffset)
				base += 0x40;
			else
				base -= 0x40;

			ctx.m_regs.SetReg(base_reg, base);

			branch = true;

			return;
		}

		u8 pre_increment = PreInc ? 4 : 0;
		u8 post_increment = !PreInc ? 4 : 0;

		if constexpr (Psr) {
			while (list) {
				if (list & 1) {
					base += pre_increment;

					ctx.m_regs.SetReg(Mode::User ,reg_id, bus->Read<u32>(base));

					base += post_increment;

					bus->m_time.access = Access::Seq;
				}

				list >>= 1;
				reg_id++;
			}
		}
		else {
			while (list) {
				if (list & 1) {
					base += pre_increment;

					ctx.m_regs.SetReg(reg_id, bus->Read<u32>(base));

					base += post_increment;

					bus->m_time.access = Access::Seq;
				}

				list >>= 1;
				reg_id++;
			}
		}

		if (CHECK_BIT(instr.rlist, 15)) {
			branch = true;

			if constexpr (Psr)
				ctx.RestorePreviousMode(ctx.m_regs.GetReg(15));
		}

		//bus->m_time.access = Access::Seq;

		bus->InternalCycles(1);
	}

	template <bool PreInc, bool AddOffset, bool Psr, bool Writeback, bool Ldm>
	void BlockDataTransfer(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		ARMBlockTransfer instr = instr_orig;

		u32 base = ctx.m_regs.GetReg(instr.base_reg);
		u32 original_base = base;

		if constexpr (Psr) {
			if (ctx.m_cpsr.mode == Mode::User ||
				ctx.m_cpsr.mode == Mode::SYS) [[unlikely]] {
					return;
			}
		}

		u8 popcnt = std::popcount(instr.rlist);

		u32 new_base = 0;

		if constexpr (!AddOffset) {
			//Registers are processed in increasing
			//addresses, which means that if we
			//are decrementing the address after
			//each load/store, we should start
			//from the lowest address and then increment
			instr.pre_increment = !instr.pre_increment;
			base -= popcnt * 4;
			new_base = base;

			constexpr bool NewInc = !PreInc;

			if constexpr (Ldm) {
				LoadBlock<NewInc, AddOffset, Psr, Writeback>(instr, ctx, bus, branch, base);
			}
			else {
				StoreBlock<NewInc, AddOffset, Psr, Writeback>(instr, ctx, bus, branch, base, new_base);
			}
		}
		else {
			new_base = base + popcnt * 4;

			if constexpr (Ldm) {
				LoadBlock<PreInc, AddOffset, Psr, Writeback>(instr, ctx, bus, branch, base);
			}
			else {
				StoreBlock<PreInc, AddOffset, Psr, Writeback>(instr, ctx, bus, branch, base, new_base);
			}
		}

		if ((instr.rlist >> instr.base_reg) & 1) {
			//Rb is in reg list
			
			if constexpr (Ldm)
				instr.writeback = false;
		}
		
		if (!instr.writeback) {
			if constexpr (!Ldm) {
				ctx.m_regs.SetReg(instr.base_reg, original_base);
			}
			else {
				if (!CHECK_BIT(instr.rlist, instr.base_reg))
					ctx.m_regs.SetReg(instr.base_reg, original_base);
			}
		}
		else if (instr.rlist)
			ctx.m_regs.SetReg(instr.base_reg, new_base);
	}

	/*
	* Data processing execution time:
	* 1S -> Prefetch
	* 
	* The execution takes longer
	* if a branch occurs (1N + 1S),
	* thanks to the pipeline flush
	* 
	* Also, if shifting by register, 
	* one internal cycle is added
	*/

	template <typename InstructionT>
	void DataProcessing(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {}

	

	template <u8 Opcode, bool S>
	void DataProcessingImm(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		static_assert(sizeof(ARM_ALUImmediate) == 4);

		ARM_ALUImmediate instr = instr_orig;
		
		u8 dest_reg = instr.dest_reg;
		u8 first_op_reg = instr.first_op_reg;

		u8 real_opcode = (instr.opcode_hi << 3) | instr.opcode_low;

		if (first_op_reg && (real_opcode == 0xD || real_opcode == 0xF)) [[unlikely]] {
			LOG_ERROR("First operand register is != 0 with MOV/MVN");
			error::DebugBreak();
		}

		u32 value = instr.immediate_value;
		u8 shift = instr.ror_shift * 2;

		bool carry_shift = CHECK_BIT(value, (shift - 1)) * !!shift;

		value = std::rotr(value, shift);

		u32 first_op = ctx.m_regs.GetReg(first_op_reg) + 8 * (first_op_reg == 15);

		detail::DataProcessingCommon<Opcode, S>(dest_reg, first_op, real_opcode, value, instr.s_bit, ctx, branch, carry_shift);

		bus->m_time.access = Access::Seq;
	}

	template <u8 Opcode, bool S, u8 ShiftType>
	void DataProcessingRegReg(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		static_assert(sizeof(ARM_ALURegisterReg) == 4);

		ARM_ALURegisterReg instr = instr_orig;
		
		u8 dest_reg = instr.destination_reg;
		u8 first_op_reg = instr.first_operand_reg;

		u8 real_opcode = (instr.opcode_hi << 3) | instr.opcode_low;

		if (first_op_reg && (real_opcode == 0xD || real_opcode == 0xF)) [[unlikely]] {
			LOG_ERROR("First operand register is != 0 with MOV/MVN");
			error::DebugBreak();
		}

		u32 second_op = ctx.m_regs.GetReg(instr.second_operand_reg) + 12 * (instr.second_operand_reg == 15);
		u32 shift_val = ctx.m_regs.GetReg(instr.shift_reg) & 0xFF;

		std::pair<u32, bool> res{};

		if constexpr (ShiftType == 0) {
			res = detail::LSL<false>(second_op, shift_val, ctx.m_cpsr.carry);
		}
		else if constexpr (ShiftType == 1) {
			res = detail::LSR<false>(second_op, shift_val, ctx.m_cpsr.carry);
		}
		else if constexpr (ShiftType == 2) {
			res = detail::ASR<false>(second_op, shift_val, ctx.m_cpsr.carry);
		}
		else if constexpr (ShiftType == 3) {
			res = detail::ROR<false>(second_op, shift_val, ctx.m_cpsr.carry);
		}

		u32 first_op = ctx.m_regs.GetReg(first_op_reg) + 12 * (first_op_reg == 15);

		bus->InternalCycles(1);

		detail::DataProcessingCommon<Opcode, S>(dest_reg, first_op,
			real_opcode, res.first, instr.s_bit, ctx,
			branch, res.second);

		bus->m_time.access = Access::Seq;
	}

	template <u8 Opcode, bool S, u8 ShiftType>
	void DataProcessingRegImm(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		static_assert(sizeof(ARM_ALURegisterImm) == 4);

		ARM_ALURegisterImm instr = instr_orig;
		
		u8 dest_reg = instr.destination_reg;
		u8 first_op_reg = instr.first_operand_reg;

		u8 real_opcode = (instr.opcode_hi << 3) | instr.opcode_low;

		if (first_op_reg && (real_opcode == 0xD || real_opcode == 0xF)) [[unlikely]] {
			LOG_ERROR("First operand register is != 0 with MOV/MVN");
			error::DebugBreak();
		}

		u8 shift_amount = instr.shift_amount_lo | 
			(instr.shift_amount_hi << 1);
		//u8 shift_type = instr.shift_type;
		u32 second_op = ctx.m_regs.GetReg( instr.second_operand_reg ) + 8 * (instr.second_operand_reg == 15);

		std::pair<u32, bool> res{};

		if constexpr (ShiftType == 0) {
			res = detail::LSL<true>(second_op, shift_amount, ctx.m_cpsr.carry);
		}
		else if constexpr (ShiftType == 1) {
			res = detail::LSR<true>(second_op, shift_amount, ctx.m_cpsr.carry);
		}
		else if constexpr (ShiftType == 2) {
			res = detail::ASR<true>(second_op, shift_amount, ctx.m_cpsr.carry);
		}
		else if constexpr (ShiftType == 3) {
			res = detail::ROR<true>(second_op, shift_amount, ctx.m_cpsr.carry);
		}

		u32 first_op = ctx.m_regs.GetReg(first_op_reg) + 8 * (first_op_reg == 15);

		detail::DataProcessingCommon<Opcode, S>(dest_reg, first_op,
			real_opcode, res.first, instr.s_bit, ctx,
			branch, res.second);

		bus->m_time.access = Access::Seq;
	}

	template <bool PreInc, bool Add, bool Imm, bool Writeback, bool Load, u8 Opcode>
	void SingleHDSTransfer(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		static_assert(sizeof(ARM_SingleHDSTransfer) == 4);

		ARM_SingleHDSTransfer instr = instr_orig;
		
		u32 offset = 0;

		if constexpr (Imm)
			offset = instr.offset_low | (instr.offset_hi << 4);
		else
			offset = ctx.m_regs.GetReg(instr.offset_low);

		u32 base = ctx.m_regs.GetReg(instr.base_reg);

		if (instr.base_reg == 15)
			base += 8;

		u8 dest = instr.source_dest_reg;

		if constexpr (PreInc) {
			if constexpr (Add)
				base += offset;
			else
				base -= offset;
		}

		base = detail::HDSTransfer<Load, Opcode>(base, dest,
			ctx, bus, branch);

		if constexpr (!PreInc) {
			if constexpr (Add)
				base += offset;
			else
				base -= offset;
		}

		if constexpr (Writeback || !PreInc) {
			if constexpr (!Load) {
				ctx.m_regs.SetReg(instr.base_reg, base);
			}
			else {
				if (instr.base_reg != instr.source_dest_reg)
					ctx.m_regs.SetReg(instr.base_reg, base);
			}
		}

		if constexpr (Load) {
			if (dest == 15) {
				branch = true;
			}
		}
	}

	void BranchExchange(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		static_assert(sizeof(ARMBranchExchange) == 4);

		ARMBranchExchange instr = instr_orig;
		
		if (instr.operand_reg == 0xF) [[unlikely]] {
			LOG_ERROR("Using r15 with BX!");
			error::DebugBreak();
		}
		
		u32 dest = ctx.m_regs.GetReg(instr.operand_reg);

		if (instr.opcode == 0x2)
			ctx.m_regs.SetReg(14, ctx.m_regs.GetReg(15) + 0x4);

		if (!(dest & 1))
			ctx.m_regs.SetReg(15, dest);
		else {
			//Switch to thumb
			ctx.m_cpsr.instr_state = InstructionMode::THUMB;

			ctx.m_regs.SetReg(15, dest - 1);
		}

		branch = true;

		bus->m_time.access = Access::Seq;
	}

	void SoftwareInterrupt(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		/*
		* Execution time 2S + 1N
		* 1S -> Prefetch
		* 1S + 1N -> Pipeline flush
		*/
		//u32 argument = instr.data & 0xFFFFFF;

		ctx.EnterException(ExceptionCode::SOFTI, 4);

		branch = true;

		bus->m_time.access = Access::Seq;
	}

	template <bool SwapByte>
	void SingleDataSwap(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		static_assert(sizeof(ARMDataSwap) == 4);

		ARMDataSwap instr = instr_orig;
		
		u32 base = ctx.m_regs.GetReg( instr.base_reg );
		u32 dest_reg = instr.dest_reg;
		u32 source_reg = instr.source_reg;

		u32 reg_value = ctx.m_regs.GetReg(source_reg);

		bus->m_time.access = Access::NonSeq;
		
		if constexpr (SwapByte) {
			u32 mem_value = bus->Read<u8>(base);
			ctx.m_regs.SetReg(dest_reg, mem_value);
			bus->Write<u8>(base, (u8)reg_value);
		}
		else {
			u32 mem_value = bus->Read<u32>(base);

			mem_value = std::rotr(mem_value,
				(base & 3) * 8);

			ctx.m_regs.SetReg(dest_reg, mem_value);
			bus->Write<u32>(base, reg_value);
		}

		bus->InternalCycles(1);
	}

	inline u32 CountZeroOneBytes(u32 value) {
		u32 copy = value >> 8;

		if (!copy || copy == 0xFFFFFF)
			return 1;

		copy >>= 8;
		
		if (!copy || copy == 0xFFFF)
			return 2;

		copy >>= 8;

		if (!copy || copy == 0xFF)
			return 3;

		return 4;
	}

	inline u32 CountZeroBytes(u32 value) {
		u32 copy = value >> 8;

		if (!copy)
			return 1;

		copy >>= 8;

		if (!copy)
			return 2;

		copy >>= 8;

		if (!copy)
			return 3;

		return 4;
	}

	template <u8 Opcode, bool S>
	void Multiply(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		static_assert(sizeof(ARMMultiply) == 4);

		ARMMultiply instr = instr_orig;

		/*
		* Execution times :
		* 1S + (opcode dependent value)
		* 
		* MUL: 
		* Add m internal cycles, where
		* m is given by the number of
		* bytes that are all zero
		* or all one
		* 
		* MLA:
		* Same as MUL but with one more I cycle
		* 
		* UMULL:
		* Add m internal cycles for
		* each byte that is all zeroes + 1I cycles
		* 
		* UMLAL
		* Same as UMULL + 1I cycle
		* cycles
		* 
		* SMULL:
		* Add m internal cycles for
		* each byte that is all zeroes or all one + 1I cycles
		* 
		* SMLAL:
		* Same as SMULL + 1I cycle
		* 
		* The u32 value that must be checked
		* for number of I cycles is the value
		* in Rs
		*/
		bus->m_time.access = Access::Seq;

		uint64_t res = 0;

		u32 icycles = 0;

		uint64_t rs = ctx.m_regs.GetReg(instr.operand_reg2);
		uint64_t rm = ctx.m_regs.GetReg(instr.operand_reg1);

		if constexpr (Opcode == 0) {
			res = rm * rs;
			icycles = CountZeroOneBytes((u32)rs);
		}
		else if constexpr (Opcode == 1) {
			res = rm * rs;
			res += (uint64_t)ctx.m_regs.GetReg(instr.accumul_reg);
			icycles = CountZeroOneBytes((u32)rs) + 1;
		}
		else if constexpr (Opcode == 4) {
			res = rm * rs;
			icycles = CountZeroBytes((u32)rs) + 1;
		}
		else if constexpr (Opcode == 5) {
			res = rm * rs;

			icycles = CountZeroBytes((u32)rs) + 2;

			uint64_t op1 = ctx.m_regs.GetReg(instr.dest_reg);
			uint64_t op2 = ctx.m_regs.GetReg(instr.accumul_reg);

			res += op2 | (op1 << 32);
		}
		else if constexpr (Opcode == 6) {
			int32_t op1 = ctx.m_regs.GetReg(instr.operand_reg1);
			int32_t op2 = ctx.m_regs.GetReg(instr.operand_reg2);

			res = (int64_t)op1 * op2;

			icycles = CountZeroOneBytes(op2) + 1;
		}
		else if constexpr (Opcode == 7) {
			int32_t op1 = ctx.m_regs.GetReg(instr.operand_reg1);
			int32_t op2 = ctx.m_regs.GetReg(instr.operand_reg2);

			res = (int64_t)op1 * op2;

			icycles = CountZeroOneBytes(op2) + 2;

			uint64_t acc1 = ctx.m_regs.GetReg(instr.dest_reg);
			uint64_t acc2 = ctx.m_regs.GetReg(instr.accumul_reg);

			res = (int64_t)res + (int64_t)(acc2 | (acc1 << 32));
		}

		if constexpr (S) {
			//ctx.m_cpsr.carry = false;
			ctx.m_cpsr.zero = (instr.opcode == 0 || instr.opcode == 1) ? !(u32)res : !res;
			ctx.m_cpsr.sign = (instr.opcode == 0 || instr.opcode == 1) ? 
				CHECK_BIT(res, 31) : CHECK_BIT(res, 63);
		}

		if (instr.dest_reg == 15) {
			LOG_INFO("Writing to r15 with multiply!");
			branch = true;
		}

		if constexpr (Opcode >= 4) {
			ctx.m_regs.SetReg(instr.accumul_reg,
				(u32)res);
			ctx.m_regs.SetReg(instr.dest_reg,
				(u32)(res >> 32));
		}
		else
			ctx.m_regs.SetReg(instr.dest_reg, (u32)res);

		bus->InternalCycles(icycles);
	}

	void MultiplyHalf(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		LOG_ERROR("Multiply half not implemented");
		error::DebugBreak();
	}

	template <bool Imm, bool PreInc, bool Add, bool Byte, bool TW, bool Load, u8 ShiftType>
	void SingleDataTransfer(ARMInstruction instr_orig, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		static_assert(sizeof(ARMSingleDataTransfer) == 4);

		if constexpr (!PreInc && TW) {
			LOG_INFO(" Unimplemented: T bit in single data transfer");
			error::DebugBreak();
		}

		ARMSingleDataTransfer instr = instr_orig;
		
		u32 offset = 0;

		if constexpr (!Imm) {
			u8 offset_reg = instr.data & 0xF;

			u8 shift_amount = (instr.data >> 7) & 0x1F;

			offset = ctx.m_regs.GetReg(offset_reg);

			if constexpr (ShiftType == 0) {
				offset = detail::LSL<true>(offset, shift_amount,
					ctx.m_cpsr.carry).first;
			}
			else if constexpr (ShiftType == 1) {
				offset = detail::LSR<true>(offset, shift_amount,
					ctx.m_cpsr.carry).first;
			}
			else if constexpr (ShiftType == 2) {
				offset = detail::ASR<true>(offset, shift_amount,
					ctx.m_cpsr.carry).first;
			}
			else if constexpr (ShiftType == 3) {
				offset = detail::ROR<true>(offset, shift_amount,
					ctx.m_cpsr.carry).first;
			}
		}
		else {
			offset = instr.data & 0xFFF;
		}

		u32 base = ctx.m_regs.GetReg(instr.base_reg) + 
			8 * (instr.base_reg == 15);

		if constexpr (PreInc) {
			if constexpr (Add)
				base += offset;
			else
				base -= offset;
		}

		if constexpr (Load) {
			u32 value = 0;

			bus->m_time.access = Access::NonSeq;

			if constexpr (Byte)
				value = bus->Read<u8>(base);
			else {
				value = bus->Read<u32>(base);
				u8 shift = (base & 3) * 8;
				value = (value >> shift) | (value << (32 - shift));
			}

			ctx.m_regs.SetReg(instr.dest_reg, value);

			bus->InternalCycles(1);
		}
		else {
			u32 value = ctx.m_regs.GetReg(instr.dest_reg) +
				12 * (instr.dest_reg == 15);

			bus->m_time.access = Access::NonSeq;

			if constexpr (Byte)
				bus->Write<u8>(base, value & 0xFF);
			else
				bus->Write<u32>(base, value);
		}

		if constexpr (!PreInc) {
			if constexpr (Add)
				base += offset;
			else
				base -= offset;
		}

		if constexpr (TW || !PreInc) {
			if constexpr (!Load) {
				ctx.m_regs.SetReg(instr.base_reg, base);
			}
			else {
				if (instr.base_reg != instr.dest_reg)
					ctx.m_regs.SetReg(instr.base_reg, base);
			}
		}

		if constexpr (Load) {
			if (instr.dest_reg == 15)
				branch = true;
		}
	}

	void Undefined(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		/*
		* Execution time 2S + 1N + 1I
		* 1S -> Prefetch
		* 1S + 1N -> Pipeline flush
		* 1I -> IDK
		* 
		* Next access type set by pipeline
		*/
		LOG_INFO("Undefined instruction at 0x{:x} : 0x{:x}", ctx.m_regs.GetReg(15), instr.data);
		
		ctx.EnterException(ExceptionCode::UNDEF, 0x4);

		branch = true;

		bus->InternalCycles(1);

		bus->m_time.access = Access::Seq;
	}

	template <u16 Instr>
	struct Decoder {
		static constexpr ARMInstructionType type = detail::arm_lookup_table[Instr];

		static constexpr ArmExecutor GetFunctionPointer() {
			constexpr u32 Low = Instr & 0xF;
			constexpr u32 High = Instr & 0xFF0;

			constexpr u32 Instruction = (High << 16) | (Low << 4);

			switch (type)
			{
			case ARMInstructionType::BRANCH: {
				constexpr bool Bl = (Instruction >> 24) & 1;
				return Branch<Bl>;
			}
			break;
			case ARMInstructionType::BRANCH_EXCHANGE: {
				return BranchExchange;
			}
			break;
			case ARMInstructionType::BLOCK_DATA_TRANSFER: {
				constexpr bool PreInc = (Instruction >> 24) & 1;
				constexpr bool AddOffset = (Instruction >> 23) & 1;
				constexpr bool Psr = (Instruction >> 22) & 1;
				constexpr bool Writeback = (Instruction >> 21) & 1;
				constexpr bool Ldm = (Instruction >> 20) & 1;

				return BlockDataTransfer<PreInc, AddOffset, Psr, Writeback, Ldm>;
			}
			break;
			case ARMInstructionType::DATA_PROCESSING_IMMEDIATE: {
				constexpr u8 Opcode = (Instruction >> 21) & 0xF;
				constexpr bool S = (Instruction >> 20) & 1;
				return DataProcessingImm<Opcode, S>;
			}
			break;
			case ARMInstructionType::DATA_PROCESSING_REGISTER_REG: {
				constexpr u8 Opcode = (Instruction >> 21) & 0xF;
				constexpr bool S = (Instruction >> 20) & 1;
				constexpr u8 Shift = (Instruction >> 5) & 0x3;
				return DataProcessingRegReg<Opcode, S, Shift>;
			}
			break;
			case ARMInstructionType::DATA_PROCESSING_REGISTER_IMM: {
				constexpr u8 Opcode = (Instruction >> 21) & 0xF;
				constexpr bool S = (Instruction >> 20) & 1;
				constexpr u8 Shift = (Instruction >> 5) & 0x3;
				return DataProcessingRegImm<Opcode, S, Shift>;
			}
			break;
			case ARMInstructionType::PSR_TRANSFER: {
				constexpr bool Imm = (Instruction >> 25) & 1;
				constexpr bool Spsr = (Instruction >> 22) & 1;
				constexpr bool Opcode = (Instruction >> 21) & 1;

				if constexpr (Opcode)
					return detail::MsrTransfer<Imm, Spsr>;
				else
					return detail::MrsTransfer<Imm, Spsr>;
			}
			break;
			case ARMInstructionType::SINGLE_HDS_TRANSFER: {
				constexpr bool PreInc = (Instruction >> 24) & 1;
				constexpr bool Add = (Instruction >> 23) & 1;
				constexpr bool Imm = (Instruction >> 22) & 1;
				constexpr bool Writeback = (Instruction >> 21) & 1;
				constexpr bool Load = (Instruction >> 20) & 1;
				constexpr u8 Opcode = (Instruction >> 5) & 0x3;
				return SingleHDSTransfer<PreInc, Add, Imm, Writeback, Load, Opcode>;
			}
			break;
			case ARMInstructionType::SOFT_INTERRUPT:
				return SoftwareInterrupt;
				break;
			case ARMInstructionType::SINGLE_DATA_SWAP: {
				constexpr bool SwpByte = (Instruction >> 22) & 1;
				return SingleDataSwap<SwpByte>;
			}
			break;
			case ARMInstructionType::MULTIPLY: {
				constexpr bool S = (Instruction >> 20) & 1;
				constexpr u8 Opcode = (Instruction >> 21) & 0x7;
				return Multiply<Opcode, S>;
			}
			break;
			case ARMInstructionType::MULTIPLY_HALF:
				return MultiplyHalf;
				break;
			case ARMInstructionType::SINGLE_DATA_TRANSFER_IMM: {
				constexpr bool Pre = (Instruction >> 24) & 1;
				constexpr bool Add = (Instruction >> 23) & 1;
				constexpr bool Byte = (Instruction >> 22) & 1;
				constexpr bool TW = (Instruction >> 21) & 1;
				constexpr bool Load = (Instruction >> 20) & 1;
				constexpr u8 Shift = (Instruction >> 5) & 0x3;
				return SingleDataTransfer<true, Pre, Add, Byte, TW, Load, Shift>;
			}
			break;
			case ARMInstructionType::SINGLE_DATA_TRANSFER: {
				constexpr bool Pre = (Instruction >> 24) & 1;
				constexpr bool Add = (Instruction >> 23) & 1;
				constexpr bool Byte = (Instruction >> 22) & 1;
				constexpr bool TW = (Instruction >> 21) & 1;
				constexpr bool Load = (Instruction >> 20) & 1;
				constexpr u8 Shift = (Instruction >> 5) & 0x3;
				return SingleDataTransfer<false, Pre, Add, Byte, TW, Load, Shift>;
			}
			break;
			case ARMInstructionType::UNDEFINED:
				return Undefined;
				break;
			default:
				return Undefined;
				break;
			}
		}
	};

	struct DecodeSeqHelper {
		template <std::size_t... Seq>
		static constexpr std::array<ArmExecutor, sizeof...(Seq)> GetTable(std::index_sequence<Seq...>) {
			return {
				Decoder<Seq>::GetFunctionPointer()...
			};
		}
	};

	template <std::size_t Max>
	struct DecodeSeq {
		static constexpr std::array<ArmExecutor, Max> CreateTable() {
			return DecodeSeqHelper::GetTable(
				std::make_index_sequence<Max>{}
			);
		}
	};

	std::array<ArmExecutor, 4096> arm_jump_table = DecodeSeq<4096>::CreateTable();

	void ExecuteArm(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		static_assert(sizeof(ARMInstruction) == 4);
		
		if (!ctx.m_cpsr.CheckCondition(instr.condition)) // The instruction takes one S cycle (caused by the fetch process)
		{
			bus->m_time.access = Access::Seq; //Set the access to sequential
			//Since no memory address was read/written
			//And no jump occurs
			return;
		}

		u16 hash = ((instr.data >> 16) & 0xFF0) | ((instr.data >> 4) & 0xF);

		arm_jump_table[hash](instr, ctx, bus, branch);
	}
}