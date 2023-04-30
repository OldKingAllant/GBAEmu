#include "../../../cpu/arm/ARM_Implementation.hpp"
#include "../../../cpu/core/CPUContext.hpp"
#include "../../../memory/Bus.hpp"

#include <bit>

#include "../../../common/Logger.hpp"
#include "../../../common/Error.hpp"
#include "../../../common/BitManip.hpp"

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

			u32 res = first_op + value + carry_val;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
				ctx.m_cpsr.CarryAdd(first_op, (uint64_t)value + carry_val);
				ctx.m_cpsr.OverflowAdd(first_op, (uint64_t)value + carry_val);
			}
		}

		void SBC(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u8 carry_val = ctx.m_cpsr.carry;

			u32 res = first_op - value + carry_val - 1;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
				ctx.m_cpsr.CarrySubtract((uint64_t)first_op + carry_val, (uint64_t)value + 1);
				ctx.m_cpsr.OverflowSubtract((uint64_t)first_op + carry_val, (uint64_t)value + 1);
			}
		}

		void RSC(u8 dest, u32 first_op, u32 value, bool s_bit, CPUContext& ctx) {
			u8 carry_val = ctx.m_cpsr.carry;

			u32 res = value - first_op + carry_val - 1;

			ctx.m_regs.SetReg(dest, res);

			if (s_bit) {
				ctx.m_cpsr.zero = !res;
				ctx.m_cpsr.sign = CHECK_BIT(res, 31);
				ctx.m_cpsr.CarrySubtract((uint64_t)value + carry_val, (uint64_t)first_op + 1);
				ctx.m_cpsr.OverflowSubtract((uint64_t)value + carry_val, (uint64_t)first_op + 1);
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
				ctx.m_cpsr.zero = !value;
				ctx.m_cpsr.sign = CHECK_BIT(value, 31);
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

		void DataProcessingCommon(u8 dest, u32 first_op, u8 opcode, u32 value, bool s_bit,
			CPUContext& ctx, bool& branch, bool shift_carry) {
			bool old_s_bit = s_bit;

			if (dest == 15 && s_bit)
				s_bit = false;

			switch (opcode)
			{
			case 0x00:
				AND(dest, first_op, value, s_bit, ctx);
				if(s_bit)
					ctx.m_cpsr.carry = shift_carry;
				break;
			case 0x01:
				EOR(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
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
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
				break;
			case 0x09:
				TEQ(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
				break;
			case 0x0A:
				CMP(dest, first_op, value, s_bit, ctx);
				break;
			case 0x0B:
				CMN(dest, first_op, value, s_bit, ctx);
				break;
			case 0x0C:
				ORR(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
				break;
			case 0x0D:
				MOV(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
				break;
			case 0x0E:
				BIC(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
				break;
			case 0x0F:
				MVN(dest, first_op, value, s_bit, ctx);
				if (s_bit)
					ctx.m_cpsr.carry = shift_carry;
				break;
			default:
				break;
			}

			if (dest != 15)
				return;

			if (old_s_bit) {
				if (ctx.m_cpsr.mode != Mode::User &&
					ctx.m_cpsr.mode != Mode::SYS) {
					ctx.RestorePreviousMode(ctx.m_regs.GetReg(15));
				}
			}

			if(opcode <= 0x7 || opcode > 0xB)
				branch = true;
			//Else, bad CMP/CMN... instruction
		}

		u32 HDSTransfer(i32 base, u8 reg, u8 opcode,
			CPUContext& ctx, memory::Bus* bus, bool& branch) {
			switch (opcode)
			{
			case 0b001: {
				u32 to_write = ctx.m_regs.GetReg(reg);
				to_write += 12 * (reg == 15);
				bus->Write<u16>(base, to_write);
			}
			break;

			case 0b010:
				LOG_ERROR("LDRD not implemented");
				error::DebugBreak();

			case 0b011:
				LOG_ERROR("STRD not implemented");
				error::DebugBreak();

			case 0b101: {
				u32 value = bus->Read<u16>(base);

				value = std::rotr(value, 8 * (base & 1));

				ctx.m_regs.SetReg(reg, value);
			}
			break;

			case 0b110: {
				int32_t value = (int8_t)bus->Read<u8>(base);
				ctx.m_regs.SetReg(reg, value);
			}
			break;

			case 0b111: {
				int32_t value = 0;

				if (base & 1)
					value = (int8_t)bus->Read<u8>(base);
				else
					value = (int16_t)bus->Read<u16>(base);

				ctx.m_regs.SetReg(reg, value);
			}
			break;

			default:
				break;
			}

			return base;
		}

		template <bool Imm>
		std::pair<u32, bool> LSL(u32 value, u8 amount, u8 old_carry) {
			if constexpr (!Imm) {
				if (!amount)
					return { value, old_carry };
			}

			u8 bit_pos = (32 - amount);

			u32 res = 0;

			if (amount >= 32) {
				u8 bit = CHECK_BIT(value, 31);
				
				for (u8 i = 0; i < 32; i++)
					res |= (bit << i);
			}
			else
				res = value << amount;

			return { res, CHECK_BIT(value, bit_pos) * (amount < 33
				&& amount) };
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

			if (amount < 32)
				res = (i32)value >> amount;
			else {
				for (int8_t pos = 31; pos >= (int8_t)(32 - amount); pos--)
					res |= (sign << pos);
			}

			/*for (int8_t pos = 31; pos >= (int8_t)(32 - amount); pos--) {
				res |= (sign << pos);
			}*/

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
				res |= (old_carry << 31);
				bit_pos = 0;
			}
			else 
				res = std::rotr(value, amount);

			return { res, CHECK_BIT(value, bit_pos) };
		}

		void MrsTransfer(ARMPsrTransferMRS instr, CPUContext& ctx) {
			if (instr.psr) {
				if (ctx.m_cpsr.mode == Mode::User ||
					ctx.m_cpsr.mode == Mode::SYS) [[unlikely]] {
				
						ctx.m_regs.SetReg(instr.dest_reg,
					ctx.m_cpsr);
						return;
				}

				ctx.m_regs.SetReg(instr.dest_reg,
					ctx.m_spsr[GetModeFromID(ctx.m_cpsr.mode)]);
			}
			else
				ctx.m_regs.SetReg(instr.dest_reg,
					ctx.m_cpsr);
		}

		void MsrTransfer(ARMPsrTransferMSR instr, CPUContext& ctx) {
			u32 value = 0;

			if (instr.immediate) {
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

			if (instr.psr) {
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

	void inline StoreBlock(ARMBlockTransfer instr, CPUContext& ctx,  memory::Bus* bus, bool& branch, u32 base, u32 new_base) {
		u16 list = instr.rlist;

		u8 reg_id = 0;

		u8 base_reg = instr.base_reg;

		if (list == 0) {
			//Empty reglist
			//The PC is saved
			//The other register are not stored
			//but the base register is modified
			//with an offset of 0x40
			i8 offset = 0;

			if (!instr.increment) {
				if (instr.pre_increment)
					offset = -0x3C;
				else
					offset = -0x40;
			}
			else {
				if (!instr.pre_increment)
					offset = 0x0;
				else
					offset = 0x4;
			}

			bus->Write<u32>((i32)base + offset, ctx.m_regs.GetReg(15) + 12);

			instr.writeback = true;
			
			if (instr.increment)
				base += 0x40;
			else
				base -= 0x40;

			ctx.m_regs.SetReg(base_reg, base);

			return;
		}

		u8 pre_increment = 4 * instr.pre_increment;
		u8 post_increment = 4 * !instr.pre_increment;

		u32 reg_value = 0;

		u8 zeroes = std::countr_zero(list);

		list >>= zeroes;
		reg_id += zeroes;

		u8 first_reg_id = reg_id;

		if (instr.s_bit) {
			if (instr.writeback) {
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
				}

				list >>= 1;
				reg_id++;
			}
		}
	}

	void inline LoadBlock(ARMBlockTransfer instr, CPUContext& ctx,  memory::Bus* bus, bool& branch, u32 base) {
		u16 list = instr.rlist;

		u8 reg_id = 0;

		u8 base_reg = instr.base_reg;

		if (list == 0) {
			i8 offset = 0;

			if (!instr.increment) {
				if (instr.pre_increment)
					offset = -0x3C;
				else
					offset = -0x40;
			}
			else {
				if (!instr.pre_increment)
					offset = 0x0;
				else
					offset = 0x4;
			}

			u32 value = bus->Read<u32>((i32)base + offset);

			ctx.m_regs.SetReg(15, value);

			instr.writeback = true;

			if (instr.increment)
				base += 0x40;
			else
				base -= 0x40;

			ctx.m_regs.SetReg(base_reg, base);

			branch = true;

			return;
		}

		u8 pre_increment = 4 * instr.pre_increment;
		u8 post_increment = 4 * !instr.pre_increment;

		if (instr.s_bit) {
			while (list) {
				if (list & 1) {
					base += pre_increment;

					ctx.m_regs.SetReg(Mode::User ,reg_id, bus->Read<u32>(base));

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

					ctx.m_regs.SetReg(reg_id, bus->Read<u32>(base));

					base += post_increment;
				}

				list >>= 1;
				reg_id++;
			}
		}

		if (CHECK_BIT(instr.rlist, 15)) {
			branch = true;

			if (instr.s_bit)
				ctx.RestorePreviousMode(ctx.m_regs.GetReg(15));
		}
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

		u8 popcnt = std::popcount(instr.rlist);

		u32 new_base = 0;

		if (!instr.increment) {
			//Registers are processed in increasing
			//addresses, which means that if we
			//are decrementing the address after
			//each load/store, we should start
			//from the lowest address and then increment
			instr.pre_increment = !instr.pre_increment;
			base -= popcnt * 4;
			new_base = base;
		}
		else {
			new_base = base + popcnt * 4;
		}

		//ctx.m_regs.SetReg(instr.base_reg, base);

		if (instr.load) {
			LoadBlock(instr, ctx, bus, branch, base);
			//Load from memory
		}
		else {
			StoreBlock(instr, ctx, bus, branch, base, new_base);
			//Write to memory
		}

		if ((instr.rlist >> instr.base_reg) & 1) {
			//Rb is in reg list
			
			if (instr.load)
				instr.writeback = false;
			/*else if (instr.writeback) {
				//If it is the first in the list
				//the value stays unchanged
				bool is_first = !(instr.rlist &
					((1 << instr.base_reg) - 1));

				if (is_first) {
					instr.writeback = false;
				}

				//Else, behaviour is not
				//specified.
			}*/
		}
		
		if (!instr.writeback) {
			if (!CHECK_BIT(instr.rlist, instr.base_reg) || !instr.load)
				ctx.m_regs.SetReg(instr.base_reg, original_base);
		}
		else if (instr.rlist)
			ctx.m_regs.SetReg(instr.base_reg, new_base);
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

		bool carry_shift = CHECK_BIT(value, (shift - 1)) * !!shift;

		value = std::rotr(value, shift);

		u32 first_op = ctx.m_regs.GetReg(first_op_reg) + 8 * (first_op_reg == 15);

		detail::DataProcessingCommon(dest_reg, first_op, real_opcode, value, instr.s_bit, ctx, branch, carry_shift);
	}

	template <>
	void DataProcessing(ARM_ALURegisterReg instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		u8 dest_reg = instr.destination_reg;
		u8 first_op_reg = instr.first_operand_reg;

		u8 real_opcode = (instr.opcode_hi << 3) | instr.opcode_low;

		if (first_op_reg && (real_opcode == 0xD || real_opcode == 0xF)) {
			LOG_ERROR("First operand register is != 0 with MOV/MVN");
			error::DebugBreak();
		}

		u32 second_op = ctx.m_regs.GetReg(instr.second_operand_reg) + 12 * (instr.second_operand_reg == 15);
		u32 shift_val = ctx.m_regs.GetReg(instr.shift_reg);

		std::pair<u32, bool> res{};

		switch (instr.shift_type)
		{
		case 0x00:
			res = detail::LSL<false>(second_op, shift_val, ctx.m_cpsr.carry);
			break;
		case 0x01:
			res = detail::LSR<false>(second_op, shift_val, ctx.m_cpsr.carry);
			break;
		case 0x02:
			res = detail::ASR<false>(second_op, shift_val, ctx.m_cpsr.carry);
			break;
		case 0x03:
			res = detail::ROR<false>(second_op, shift_val, ctx.m_cpsr.carry);
			break;
		default:
			break;
		}

		u32 first_op = ctx.m_regs.GetReg(first_op_reg) + 12 * (first_op_reg == 15);

		detail::DataProcessingCommon(dest_reg, first_op,
			real_opcode, res.first, instr.s_bit, ctx,
			branch, res.second);
	}

	template <>
	void DataProcessing(ARM_ALURegisterImm instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		u8 dest_reg = instr.destination_reg;
		u8 first_op_reg = instr.first_operand_reg;

		u8 real_opcode = (instr.opcode_hi << 3) | instr.opcode_low;

		if (first_op_reg && (real_opcode == 0xD || real_opcode == 0xF)) {
			LOG_ERROR("First operand register is != 0 with MOV/MVN");
			error::DebugBreak();
		}

		u8 shift_amount = instr.shift_amount_lo | 
			(instr.shift_amount_hi << 1);
		u8 shift_type = instr.shift_type;
		u32 second_op = ctx.m_regs.GetReg( instr.second_operand_reg ) + 8 * (instr.second_operand_reg == 15);

		std::pair<u32, bool> res{};

		switch (shift_type)
		{
		case 0x00:
			res = detail::LSL<true>(second_op, shift_amount, ctx.m_cpsr.carry);
			break;
		case 0x01:
			res = detail::LSR<true>(second_op, shift_amount, ctx.m_cpsr.carry);
			break;
		case 0x02:
			res = detail::ASR<true>(second_op, shift_amount, ctx.m_cpsr.carry);
			break;
		case 0x03:
			res = detail::ROR<true>(second_op, shift_amount, ctx.m_cpsr.carry);
			break;
		default:
			break;
		}

		u32 first_op = ctx.m_regs.GetReg(first_op_reg) + 8 * (first_op_reg == 15);

		detail::DataProcessingCommon(dest_reg, first_op,
			real_opcode, res.first, instr.s_bit, ctx,
			branch, res.second);
	}

	void PsrTransfer(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		if (CHECK_BIT(instr.data, 21))
			detail::MsrTransfer(instr, ctx);
		else
			detail::MrsTransfer(instr, ctx);
	}

	void SingleHDSTransfer(ARM_SingleHDSTransfer instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		u32 offset = 0;

		if (instr.immediate_offset)
			offset = instr.offset_low | (instr.offset_hi << 4);
		else
			offset = ctx.m_regs.GetReg(instr.offset_low);

		u32 base = ctx.m_regs.GetReg(instr.base_reg);

		if (instr.base_reg == 15)
			base += 8;

		u8 dest = instr.source_dest_reg;

		u8 opcode = ((u8)instr.load << 2) | instr.opcode;

		if (instr.pre_inc) {
			if (instr.increment)
				base += offset;
			else
				base -= offset;
		}

		base = detail::HDSTransfer(base, dest, opcode,
			ctx, bus, branch);

		if (!instr.pre_inc) {
			if (instr.increment)
				base += offset;
			else
				base -= offset;
		}

		if (instr.writeback || !instr.pre_inc) {
			if (!instr.load || (instr.base_reg != instr.source_dest_reg))
				ctx.m_regs.SetReg(instr.base_reg, base);
		}
	}

	void BranchExchange(ARMBranchExchange instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		if (instr.operand_reg == 0xF) {
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
	}

	void SoftwareInterrupt(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		LOG_ERROR("Software interrupt not implemented");
		error::DebugBreak();
	}

	void SingleDataSwap(ARMDataSwap instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		u32 base = ctx.m_regs.GetReg( instr.base_reg );
		u32 dest_reg = instr.dest_reg;
		u32 source_reg = instr.source_reg;

		u32 reg_value = ctx.m_regs.GetReg(source_reg);
		
		if (instr.swap_byte) {
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
	}

	void Multiply(ARMMultiply instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		uint64_t res = 0;

		switch (instr.opcode)
		{
		case 0x0:
			res = (uint64_t)ctx.m_regs.GetReg(instr.operand_reg1) *
				ctx.m_regs.GetReg(instr.operand_reg2);
			break;

		case 0x1:
			res = (uint64_t)ctx.m_regs.GetReg(instr.operand_reg1) *
				ctx.m_regs.GetReg(instr.operand_reg2);

			if (instr.accumul_reg)
				res += (uint64_t)ctx.m_regs.GetReg(instr.accumul_reg);
			break;

		case 0x4:
			res = (uint64_t)ctx.m_regs.GetReg(instr.operand_reg1) *
				ctx.m_regs.GetReg(instr.operand_reg2);
			break;

		case 0x5:
			res = (uint64_t)ctx.m_regs.GetReg(instr.operand_reg1) *
				ctx.m_regs.GetReg(instr.operand_reg2);

			if (instr.accumul_reg) {
				uint64_t op1 = ctx.m_regs.GetReg(instr.dest_reg);
				uint64_t op2 = ctx.m_regs.GetReg(instr.accumul_reg);

				res += op2 | (op1 << 32);
			}
			break;

		case 0x6: {
			int32_t op1 = ctx.m_regs.GetReg(instr.operand_reg1);
			int32_t op2 = ctx.m_regs.GetReg(instr.operand_reg2);
			
			res = (int64_t)op1 * op2;
		}
		break;

		case 0x7: {
			int32_t op1 = ctx.m_regs.GetReg(instr.operand_reg1);
			int32_t op2 = ctx.m_regs.GetReg(instr.operand_reg2);

			res = (int64_t)op1 * op2;

			if (instr.accumul_reg) {
				uint64_t op1 = ctx.m_regs.GetReg(instr.dest_reg);
				uint64_t op2 = ctx.m_regs.GetReg(instr.accumul_reg);

				res = (int64_t)res + (int64_t)(op2 | (op1 << 32));
			}
		}
		break;

		default:
			LOG_ERROR("Invalid multiply type");
			error::DebugBreak();
		}

		if (instr.s_bit) {
			ctx.m_cpsr.carry = false;
			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
		}

		if (instr.opcode >= 4) {
			ctx.m_regs.SetReg(instr.accumul_reg,
				(u32)res);
			ctx.m_regs.SetReg(instr.dest_reg,
				(u32)(res >> 32));
		}
		else
			ctx.m_regs.SetReg(instr.dest_reg, (u32)res);
	}

	void MultiplyHalf(ARMInstruction instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		LOG_ERROR("Multiply half not implemented");
		error::DebugBreak();
	}

	template <bool Imm>
	void SingleDataTransfer(ARMSingleDataTransfer instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		u32 offset = 0;

		if constexpr (!Imm) {
			u8 offset_reg = instr.data & 0xF;

			u8 shift_amount = (instr.data >> 7) & 0x1F;

			offset = ctx.m_regs.GetReg(offset_reg);

			switch ((instr.data >> 5) & 0x3)
			{
			case 0x0:
				offset = detail::LSL<true>(offset, shift_amount, 
					ctx.m_cpsr.carry).first;
				break;
			case 0x1:
				offset = detail::LSR<true>(offset, shift_amount,
					ctx.m_cpsr.carry).first;
				break;
			case 0x2:
				offset = detail::ASR<true>(offset, shift_amount,
					ctx.m_cpsr.carry).first;
				break;
			case 0x3:
				offset = detail::ROR<true>(offset, shift_amount,
					ctx.m_cpsr.carry).first;
				break;
			}
		}
		else {
			offset = instr.data & 0xFFF;
		}

		u32 base = ctx.m_regs.GetReg(instr.base_reg) + 
			8 * (instr.base_reg == 15);

		if (instr.pre_incre) {
			if (instr.increment)
				base += offset;
			else
				base -= offset;
		}

		if (instr.load) {
			u32 value = 0;

			if (instr.move_byte)
				value = bus->Read<u8>(base);
			else 
				value = bus->Read<u32>(base);

			value = std::rotr(value, (base & 3) * 8);

			ctx.m_regs.SetReg(instr.dest_reg, value);
		}
		else {
			u32 value = ctx.m_regs.GetReg(instr.dest_reg) +
				12 * (instr.dest_reg == 15);

			if (instr.move_byte)
				bus->Write<u8>(base, value & 0xFF);
			else
				bus->Write<u32>(base, value);
		}

		if (!instr.pre_incre) {
			if (instr.increment)
				base += offset;
			else
				base -= offset;
		}

		if (instr.writeback_or_t || !instr.pre_incre) {
			if(!instr.load || (instr.base_reg != instr.dest_reg))
				ctx.m_regs.SetReg(instr.base_reg, base);
		}
		
		if (instr.dest_reg == 15 && instr.load)
			branch = true;
	}

	void Undefined(ARMInstruction instr, CPUContext&, memory::Bus*, bool& branch) {
		LOG_ERROR("Undefined instruction");
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
			BranchExchange(instr, ctx, bus, branch);
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
		case ARMInstructionType::SOFT_INTERRUPT:
			SoftwareInterrupt(instr, ctx, bus, branch);
			break;
		case ARMInstructionType::SINGLE_DATA_SWAP:
			SingleDataSwap(instr, ctx, bus, branch);
			break;
		case ARMInstructionType::MULTIPLY:
			Multiply(instr, ctx, bus, branch);
			break;
		case ARMInstructionType::MULTIPLY_HALF:
			MultiplyHalf(instr, ctx, bus, branch);
			break;
		case ARMInstructionType::SINGLE_DATA_TRANSFER_IMM:
			SingleDataTransfer<true>(instr, ctx, bus, branch);
			break;
		case ARMInstructionType::SINGLE_DATA_TRANSFER:
			SingleDataTransfer<false>(instr, ctx, bus, branch);
			break;
		case ARMInstructionType::UNDEFINED:
			Undefined(instr, ctx, bus, branch);
			break;
		default:
			break;
		}
	}
}