#include "../../../cpu/thumb/THUMB_Implementation.hpp"

#include "../../../common/Logger.hpp"
#include "../../../common/BitManip.hpp"
#include "../../../common/Error.hpp"

#include "../../../cpu/thumb/TableGen2.hpp"

#include <bit>

namespace GBA::cpu::thumb{
	LOG_CONTEXT(THUMB_Interpreter);

	namespace detail {
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

		void AND(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);

			reg_value &= source;

			ctx.m_regs.SetReg(dest_reg, reg_value);

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
		}

		void EOR(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);

			reg_value ^= source;

			ctx.m_regs.SetReg(dest_reg, reg_value);

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
		}

		void ADC(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);
			u32 original = reg_value;

			u8 carry = ctx.m_cpsr.carry;

			reg_value += source + carry;

			ctx.m_regs.SetReg(dest_reg, reg_value);

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
			ctx.m_cpsr.CarryAdd(original, (uint64_t)source + carry);
			ctx.m_cpsr.OverflowAdd(original, (uint64_t)source + carry);
		}

		void SBC(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);
			u32 original = reg_value;

			u8 carry = ctx.m_cpsr.carry;

			reg_value = reg_value - source - !carry;

			ctx.m_regs.SetReg(dest_reg, reg_value);

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
			ctx.m_cpsr.CarrySubtract(original, (uint64_t)source + !carry);
			ctx.m_cpsr.OverflowSubtract(original, (uint64_t)source + !carry);
		}

		void TST(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);

			reg_value &= source;

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
		}

		void NEG(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 original = source;

			u32 reg_value = 0 - source;

			ctx.m_regs.SetReg(dest_reg, reg_value);

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);

			ctx.m_cpsr.CarrySubtract(0, original);
			ctx.m_cpsr.OverflowSubtract(0, original);
		}

		void CMP(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);
			u32 original = reg_value;

			reg_value = reg_value - source;

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
			ctx.m_cpsr.CarrySubtract(original, source);
			ctx.m_cpsr.OverflowSubtract(original, source);
		}

		void CMN(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);
			u32 original = reg_value;

			reg_value = reg_value + source;

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
			ctx.m_cpsr.CarryAdd(original, source);
			ctx.m_cpsr.OverflowAdd(original, source);
		}

		void ORR(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);

			reg_value |= source;

			ctx.m_regs.SetReg(dest_reg, reg_value);

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
		}

		void MUL(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);

			reg_value *= source;

			ctx.m_regs.SetReg(dest_reg, reg_value);

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
			ctx.m_cpsr.carry = false;
		}

		void BIC(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);

			reg_value = reg_value & ~source;

			ctx.m_regs.SetReg(dest_reg, reg_value);

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
		}

		void MVN(u8 dest_reg, u32 source, CPUContext& ctx) {
			u32 reg_value = ctx.m_regs.GetReg(dest_reg);

			reg_value = ~source;

			ctx.m_regs.SetReg(dest_reg, reg_value);

			ctx.m_cpsr.zero = !reg_value;
			ctx.m_cpsr.sign = CHECK_BIT(reg_value, 31);
		}

		void Push(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool&) {
			bool lr_pc = CHECK_BIT(instr, 8);
			u8 rlist = instr & 0xFF;

			u32 base = ctx.m_regs.GetReg(13);

			base -= 4 * std::popcount(rlist);

			if (lr_pc) {
				base -= 4;
			}

			u32 new_base = base;
			u8 reg_id = 0;

			bus->m_time.access = Access::NonSeq;

			while (rlist) {
				if (rlist & 1) {
					bus->Write<u32>(base, ctx.m_regs.GetReg(reg_id));
					base += 4;
					bus->m_time.access = Access::Seq;
				}

				reg_id++;
				rlist >>= 1;
			}

			if (lr_pc) {
				bus->Write<u32>(base, ctx.m_regs.GetReg(14));
				base += 4;
			}

			bus->m_time.access = Access::NonSeq;

			ctx.m_regs.SetReg(13, new_base);
		}

		void Pop(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
			bool lr_pc = CHECK_BIT(instr, 8);

			u8 rlist = instr & 0xFF;
			u32 base = ctx.m_regs.GetReg(13);
			u8 reg_id = 0;

			bus->m_time.access = Access::NonSeq;

			while (rlist) {
				if (rlist & 1) {
					u32 value = bus->Read<u32>(base);
					ctx.m_regs.SetReg(reg_id, value);
					base += 4;
					bus->m_time.access = Access::Seq;
				}

				reg_id++;
				rlist >>= 1;
			}

			if (lr_pc) {
				u32 value = bus->Read<u32>(base);
				ctx.m_regs.SetReg(15, value);
				base += 4;
				branch = true;
			}

			bus->m_time.access = Access::Seq;

			bus->InternalCycles(1);

			ctx.m_regs.SetReg(13, base);
		}

		void StoreMultiple(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
			u8 base_reg = (instr >> 8) & 0x7;

			u8 rlist = instr & 0xFF;

			u32 base = ctx.m_regs.GetReg(base_reg);
			u32 orig_base = base;
			u32 new_base = base + std::popcount(rlist) * 4;

			bus->m_time.access = Access::NonSeq;

			if (!rlist) {
				u32 value = ctx.m_regs.GetReg(15) + 6;
				bus->Write<u32>(base, value);

				ctx.m_regs.SetReg(base_reg, base + 0x40);
				return;
			}
			
			u8 reg_id = 0;

			while (rlist) {
				if (rlist & 1) {
					if (reg_id == base_reg) {
						if (base == orig_base)
							bus->Write<u32>(base, orig_base);
						else
							bus->Write<u32>(base, new_base);
					}
					else
						bus->Write<u32>(base, ctx.m_regs.GetReg(reg_id));

					base += 4;

					bus->m_time.access = Access::Seq;
				}

				reg_id++;
				rlist >>= 1;
			}

			bus->m_time.access = Access::NonSeq;

			ctx.m_regs.SetReg(base_reg, base);
		}

		void LoadMultiple(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
			u8 base_reg = (instr >> 8) & 0x7;

			u8 rlist = instr & 0xFF;

			u32 base = ctx.m_regs.GetReg(base_reg);
			u32 orig_base = base;
			u32 new_base = base + std::popcount(rlist) * 4;

			bus->m_time.access = Access::NonSeq;

			if (!rlist) {
				u32 value = bus->Read<u32>(base);
				ctx.m_regs.SetReg(15, value);
				branch = true;
				ctx.m_regs.SetReg(base_reg, base + 0x40);
				return;
			}

			u8 reg_id = 0;

			while (rlist) {
				if (rlist & 1) {
					u32 value = bus->Read<u32>(base);

					ctx.m_regs.SetReg(reg_id, value);

					base += 4;

					bus->m_time.access = Access::Seq;
				}

				reg_id++;
				rlist >>= 1;
			}

			rlist = instr & 0xFF;

			bus->m_time.access = Access::Seq;

			bus->InternalCycles(1);

			if(!CHECK_BIT(rlist, base_reg))
				ctx.m_regs.SetReg(base_reg, base);
		}
	}

	ThumbFunc THUMB_JUMP_TABLE[20] = {};

	void ThumbUndefined(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		ctx.EnterException(ExceptionCode::UNDEF, 2);

		branch = true;

		bus->m_time.access = Access::NonSeq;
	}

	void ThumbFormat3(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool&) {
		//Execution time : 1S
		u8 immediate = instr & 0xFF;
		u8 dest_reg = (instr >> 8) & 0x7;

		switch ((instr >> 11) & 0x3)
		{
		case 0x00: {
			ctx.m_regs.SetReg(dest_reg, immediate);
			ctx.m_cpsr.sign = CHECK_BIT(immediate, 7);
			ctx.m_cpsr.zero = !immediate;
		}
			break;
		case 0x01: {
			u32 reg_val = ctx.m_regs.GetReg(dest_reg);
			u32 res = reg_val - immediate;
			//Trash the result
			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.CarrySubtract(reg_val, immediate);
			ctx.m_cpsr.OverflowSubtract(reg_val, immediate);
		}
			break;
		case 0x02 : {
			u32 reg_val = ctx.m_regs.GetReg(dest_reg);
			u32 res = reg_val + immediate;

			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.CarryAdd(reg_val, immediate);
			ctx.m_cpsr.OverflowAdd(reg_val, immediate);

			ctx.m_regs.SetReg(dest_reg, res);
		}
			break;
		case 0x03: {
			u32 reg_val = ctx.m_regs.GetReg(dest_reg);
			u32 res = reg_val - immediate;

			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.CarrySubtract(reg_val, immediate);
			ctx.m_cpsr.OverflowSubtract(reg_val, immediate);

			ctx.m_regs.SetReg(dest_reg, res);
		}
			break;
		}

		bus->m_time.access = Access::Seq;
	}

	void ThumbFormat5(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		//Execution time :
		//1S for normal
		//2S + 1N for branches
		u8 source_reg = (instr >> 3) & 0xF;
		u8 dest_reg = (instr & 0x7) | ((instr >> 4) &
			0b1000);

		u8 opcode = (instr >> 8) & 0x3;

		u32 source = ctx.m_regs.GetReg(source_reg);
		source += 4 * (source_reg == 15);

		switch (opcode)
		{
		case 0x00: {
			u32 first_op = ctx.m_regs.GetReg(dest_reg) + 
				4 * (dest_reg == 15);

			ctx.m_regs.SetReg(dest_reg, first_op + source);
		}
			break;
		case 0x01: {
			u32 first_op = ctx.m_regs.GetReg(dest_reg) +
				4 * (dest_reg == 15);

			u32 res = first_op - source;

			ctx.m_cpsr.sign = CHECK_BIT(res, 31);
			ctx.m_cpsr.zero = !res;
			ctx.m_cpsr.CarrySubtract(first_op, source);
			ctx.m_cpsr.OverflowSubtract(first_op, source);
		}
			break;
		case 0x02: {
			ctx.m_regs.SetReg(dest_reg, source);
		}
			break;
		case 0x03: {
			if (!(source & 1)) {
				ctx.m_cpsr.instr_state = InstructionMode::ARM;

				if (source_reg == 15)
					source &= ~2;

				ctx.m_regs.SetReg(15, source);
			}
			else {
				ctx.m_regs.SetReg(15, source);
			}

			branch = true;

			bus->m_time.access = Access::NonSeq;
		}
			break;
		default:
			break;
		}

		if (dest_reg == 15) {
			branch = true;
			bus->m_time.access = Access::NonSeq;
		}
		else if (opcode != 0x3)
			bus->m_time.access = Access::Seq;
			
	}

	void ThumbFormat12(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		//Execution time : 1S
		u32 source = 0;

		if (CHECK_BIT(instr, 11))
			source = ctx.m_regs.GetReg(13);
		else
			source = (ctx.m_regs.GetReg(15) + 4) & ~2;

		u8 dest_reg = (instr >> 8) & 0x7;
		u32 offset = (instr & 0xFF) * 4;

		ctx.m_regs.SetReg(dest_reg, source + offset);

		bus->m_time.access = Access::Seq;
	}

	void ThumbFormat16(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		//Execution time :
		//1S if cond is false
		//2S + 1N if cond is true
		u8 opcode = (instr >> 8) & 0xF;

		if (!ctx.m_cpsr.CheckCondition(opcode)) {
			bus->m_time.access = Access::Seq;
			return;
		}

		i16 offset = instr & 0xFF;

		offset <<= 8;
		offset >>= 8;

		offset *= 2;

		ctx.m_regs.AddOffset(15, 4 + offset);

		branch = true;

		bus->m_time.access = Access::NonSeq;
	}

	void ThumbFormat18(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		//Execution time : 2S + 1N
		i16 offset = instr & 0b11111111111;

		offset <<= 5;
		offset >>= 5;

		offset *= 2;

		ctx.m_regs.AddOffset(15, 4 + offset);

		branch = true;

		bus->m_time.access = Access::NonSeq;
	}

	void ThumbFormat1(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		//Execution time : 1S
		u8 opcode = (instr >> 11) & 0x3;

		u8 shift = (instr >> 6) & 0x1F;
		u8 source = (instr >> 3) & 0x7;
		u8 dest = instr & 0x7;

		bool carry = ctx.m_cpsr.carry;

		std::pair<u32, bool> res{};

		u32 source_val = ctx.m_regs.GetReg(source);

		switch (opcode)
		{
		case 0x0:
			res = detail::LSL<true>(source_val, shift, carry);
			break;

		case 0x1:
			res = detail::LSR<true>(source_val, shift, carry);
			break;

		case 0x2:
			res = detail::ASR<true>(source_val, shift, carry);
			break;

		default:
			error::Unreachable();
			break;
		}

		ctx.m_regs.SetReg(dest, res.first);

		ctx.m_cpsr.zero = !res.first;
		ctx.m_cpsr.sign = CHECK_BIT(res.first, 31);
		ctx.m_cpsr.carry = res.second;

		bus->m_time.access = Access::Seq;
	}

	void ThumbFormat2(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		//Execution time : 1S
		u8 opcode = (instr >> 9) & 0x3;

		u8 reg_or_imm = (instr >> 6) & 0x7;

		u8 source_reg = (instr >> 3) & 0x7;
		u8 dest_reg = instr & 0x7;

		u32 res = 0;

		u32 source = ctx.m_regs.GetReg(source_reg);

		switch (opcode)
		{
		case 0x0: {
			u32 operand = ctx.m_regs.GetReg(reg_or_imm);
			res = source + operand;
			ctx.m_cpsr.CarryAdd(source, operand);
			ctx.m_cpsr.OverflowAdd(source, operand);
		}
		break;

		case 0x1: {
			u32 operand = ctx.m_regs.GetReg(reg_or_imm);
			res = source - operand;
			ctx.m_cpsr.CarrySubtract(source, operand);
			ctx.m_cpsr.OverflowSubtract(source, operand);
		}
		break;

		case 0x2: {
			u32 operand = reg_or_imm;
			res = source + operand;
			ctx.m_cpsr.CarryAdd(source, operand);
			ctx.m_cpsr.OverflowAdd(source, operand);
		}
		break;

		case 0x3: {
			u32 operand = reg_or_imm;
			res = source - operand;
			ctx.m_cpsr.CarrySubtract(source, operand);
			ctx.m_cpsr.OverflowSubtract(source, operand);
		}
		break;

		default:
			error::Unreachable();
			break;
		}

		ctx.m_cpsr.zero = !res;
		ctx.m_cpsr.sign = CHECK_BIT(res, 31);

		ctx.m_regs.SetReg(dest_reg, res);

		bus->m_time.access = Access::Seq;
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

	void ThumbFormat4(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		//Execution time :
		//1S +
		//1I if shift
		//mI if MUL
		u8 opcode = (instr >> 6) & 0xF;

		u8 source_reg = (instr >> 3) & 0x7;
		u8 dest_reg = instr & 0x7;

		u32 source_val = ctx.m_regs.GetReg(source_reg);

		u8 shift = source_val & 0xFF;

		std::pair<u32, bool> shift_res{};

		bool carry = ctx.m_cpsr.carry;

		u32 dest_val = ctx.m_regs.GetReg(dest_reg);

		switch (opcode)
		{
		case 0x0:
			detail::AND(dest_reg, source_val, ctx);
			break;

		case 0x1:
			detail::EOR(dest_reg, source_val, ctx);
			break;

		case 0x2: {
			shift_res = detail::LSL<false>(dest_val, shift, carry);

			ctx.m_regs.SetReg(dest_reg, shift_res.first);

			ctx.m_cpsr.carry = shift_res.second;
			ctx.m_cpsr.zero = !shift_res.first;
			ctx.m_cpsr.sign = CHECK_BIT(shift_res.first, 31);

			bus->InternalCycles(1);
		}
		break;

		case 0x3: {
			shift_res = detail::LSR<false>(dest_val, shift, carry);

			ctx.m_regs.SetReg(dest_reg, shift_res.first);

			ctx.m_cpsr.carry = shift_res.second;
			ctx.m_cpsr.zero = !shift_res.first;
			ctx.m_cpsr.sign = CHECK_BIT(shift_res.first, 31);
		
			bus->InternalCycles(1);
		}
		break;

		case 0x4: {
			shift_res = detail::ASR<false>(dest_val, shift, carry);

			ctx.m_regs.SetReg(dest_reg, shift_res.first);

			ctx.m_cpsr.carry = shift_res.second;
			ctx.m_cpsr.zero = !shift_res.first;
			ctx.m_cpsr.sign = CHECK_BIT(shift_res.first, 31);
		
			bus->InternalCycles(1);
		}
		break;

		case 0x5:
			detail::ADC(dest_reg, source_val, ctx);
			break;

		case 0x6:
			detail::SBC(dest_reg, source_val, ctx);
			break;

		case 0x7: {
			shift_res = detail::ROR<false>(dest_val, shift, carry);

			ctx.m_regs.SetReg(dest_reg, shift_res.first);

			ctx.m_cpsr.carry = shift_res.second;
			ctx.m_cpsr.zero = !shift_res.first;
			ctx.m_cpsr.sign = CHECK_BIT(shift_res.first, 31);
		
			bus->InternalCycles(1);
		}
		break;

		case 0x8:
			detail::TST(dest_reg, source_val, ctx);
			break;

		case 0x9:
			detail::NEG(dest_reg, source_val, ctx);
			break;

		case 0xA:
			detail::CMP(dest_reg, source_val, ctx);
			break;

		case 0xB:
			detail::CMN(dest_reg, source_val, ctx);
			break;

		case 0xC:
			detail::ORR(dest_reg, source_val, ctx);
			break;

		case 0xD: {
			detail::MUL(dest_reg, source_val, ctx);

			u8 cycles = CountZeroOneBytes(
				ctx.m_regs.GetReg(dest_reg)
			);

			bus->InternalCycles(cycles);
		}
		break;

		case 0xE:
			detail::BIC(dest_reg, source_val, ctx);
			break;

		case 0xF:
			detail::MVN(dest_reg, source_val, ctx);
			break;

		default:
			error::Unreachable();
			break;
		}

		bus->m_time.access = Access::Seq;
	}

	void ThumbFormat13(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		bool opcode = CHECK_BIT(instr, 7);

		u16 offset = (instr & 0x7F) * 4;

		if (opcode)
			ctx.m_regs.AddOffset(13, -offset);
		else
			ctx.m_regs.AddOffset(13, offset);

		bus->m_time.access = Access::Seq;
	}

	void ThumbFormat19(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		u32 curr_opcode = (instr >> 11) & 0x1F;

		if (curr_opcode == 0x1E) {
			i32 addr_upper = instr & 0b11111111111;

			addr_upper <<= 21;
			addr_upper >>= 21;

			u32 pc = ctx.m_regs.GetReg(15);

			addr_upper = pc + 0x4 + (addr_upper << 12);

			ctx.m_regs.SetReg(14, addr_upper);

			bus->m_time.access = Access::Seq;

			return;
		}
		
		if (curr_opcode != 0x1F)
			error::DebugBreak();

		u32 addr_low = instr & 0b11111111111;

		u32 new_pc = 0;

		new_pc = ctx.m_regs.GetReg(14) + (addr_low << 1);

		u32 new_lr = (ctx.m_regs.GetReg(15) + 2) | 1;

		ctx.m_regs.SetReg(15, new_pc);
		ctx.m_regs.SetReg(14, new_lr);

		branch = true;

		bus->m_time.access = Access::NonSeq;
	}

	void ThumbFormat6(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		u8 dest_reg = (instr >> 8) & 0x7;

		u16 offset = (instr & 0xFF) * 4;

		u32 pc = (ctx.m_regs.GetReg(15) + 4) & ~2;

		u32 address = pc + offset;

		bus->m_time.access = Access::NonSeq;

		u32 value = bus->Read<u32>(address);

		bus->m_time.access = Access::Seq;

		value = std::rotr(value, (address & 3) * 8);

		ctx.m_regs.SetReg(dest_reg, value);

		bus->InternalCycles(1);
	}

	void ThumbFormat7(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		u8 type = (instr >> 10) & 0x3;

		u8 offset_reg = (instr >> 6) & 0x7;
		u8 base_reg = (instr >> 3) & 0x7;
		u8 rd = instr & 0x7;

		u32 address = ctx.m_regs.GetReg(base_reg)
			+ ctx.m_regs.GetReg(offset_reg);

		bus->m_time.access = Access::NonSeq;

		switch (type)
		{
		case 0x0: {
			u32 value = ctx.m_regs.GetReg(rd);
			bus->Write<u32>(address, value);
		}
		break;

		case 0x1: {
			u32 value = ctx.m_regs.GetReg(rd);
			bus->Write<u8>(address, (u8)value);
		}
		break;

		case 0x2: {
			u32 value = bus->Read<u32>(address);
			value = std::rotr(value, (address & 3) * 8);
			ctx.m_regs.SetReg(rd, value);
			bus->m_time.access = Access::Seq;
			bus->InternalCycles(1);
		}
		break;

		case 0x3: {
			u32 value = bus->Read<u8>(address);
			ctx.m_regs.SetReg(rd, value);
			bus->m_time.access = Access::Seq;
			bus->InternalCycles(1);
		}
		break;

		default:
			error::DebugBreak();
			break;
		}
	}

	void ThumbFormat8(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		u8 type = (instr >> 10) & 0x3;

		u8 offset_reg = (instr >> 6) & 0x7;
		u8 base_reg = (instr >> 3) & 0x7;
		u8 rd = instr & 0x7;

		u32 address = ctx.m_regs.GetReg(base_reg)
			+ ctx.m_regs.GetReg(offset_reg);

		bus->m_time.access = Access::NonSeq;

		switch (type)
		{
		case 0x0: {
			u32 value = ctx.m_regs.GetReg(rd);
			bus->Write<u16>(address, (u16)value);
		}
		break;

		case 0x1: {
			i32 value = (i8)bus->Read<u8>(address);
			ctx.m_regs.SetReg(rd, value);
			bus->m_time.access = Access::Seq;
			bus->InternalCycles(1);
		}
		break;

		case 0x2: {
			u32 value = bus->Read<u16>(address);
			value = std::rotr(value, (address & 1) * 8);
			ctx.m_regs.SetReg(rd, value);
			bus->m_time.access = Access::Seq;
			bus->InternalCycles(1);
		}
		break;

		case 0x3: {
			i32 value = 0;

			if (address & 1)
				value = (i8)bus->Read<u8>(address);
			else
				value = (i16)bus->Read<u16>(address);

			ctx.m_regs.SetReg(rd, value);
			bus->m_time.access = Access::Seq;
			bus->InternalCycles(1);
		}
		break;

		default:
			error::DebugBreak();
			break;
		}
	}

	void ThumbFormat9(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		u8 type = (instr >> 11) & 0x3;

		u32 offset = (instr >> 6) & 0x1F;
		u8 base_reg = (instr >> 3) & 0x7;
		u8 rd = instr & 0x7;

		u32 address = ctx.m_regs.GetReg(base_reg);

		bus->m_time.access = Access::NonSeq;

		switch (type)
		{
		case 0x0: {
			address += offset * 4;
			u32 value = ctx.m_regs.GetReg(rd);
			bus->Write<u32>(address, value);
		}
		break;

		case 0x1: {
			address += offset * 4;
			u32 value = bus->Read<u32>(address);
			value = std::rotr(value, (address & 3) * 8);
			ctx.m_regs.SetReg(rd, value);
			bus->m_time.access = Access::Seq;
			bus->InternalCycles(1);
		}
		break;

		case 0x2: {
			u32 value = ctx.m_regs.GetReg(rd);
			bus->Write<u8>(address + offset, (u8)value);
		}
		break;

		case 0x3: {
			u8 value = bus->Read<u8>(address + offset);

			ctx.m_regs.SetReg(rd, value);
			bus->m_time.access = Access::Seq;
			bus->InternalCycles(1);
		}
		break;

		default:
			error::DebugBreak();
			break;
		}
	}

	void ThumbFormat10(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		bool type = CHECK_BIT(instr, 11);

		u32 offset = ((instr >> 6) & 0x1F) * 2;

		u8 base_reg = (instr >> 3) & 0x7;
		u8 rd = instr & 0x7;

		u32 address = ctx.m_regs.GetReg(base_reg) + offset;

		bus->m_time.access = Access::NonSeq;

		if (!type) {
			bus->Write<u16>(address, (u16)ctx.m_regs.GetReg(rd));
		}
		else {
			u32 value = bus->Read<u16>(address);
			value = std::rotr(value, (address & 1) * 8);

			ctx.m_regs.SetReg(rd, value);

			bus->m_time.access = Access::Seq;
			bus->InternalCycles(1);
		}
	}

	void ThumbFormat11(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		bool type = CHECK_BIT(instr, 11);

		u8 rd = (instr >> 8) & 0x7;

		u16 offset = (instr & 0xFF) * 4;

		u32 address = ctx.m_regs.GetReg(13) + offset;

		bus->m_time.access = Access::NonSeq;

		if (!type) {
			u32 value = ctx.m_regs.GetReg(rd);
			bus->Write<u32>(address, value);
		}
		else {
			u32 value = bus->Read<u32>(address);
			value = std::rotr(value, (address & 3) * 8);
			ctx.m_regs.SetReg(rd, value);

			bus->m_time.access = Access::Seq;
			bus->InternalCycles(1);
		}
	}

	void ThumbFormat14(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		bool type = CHECK_BIT(instr, 11);

		if (type)
			detail::Pop(instr, bus, ctx, branch);
		else
			detail::Push(instr, bus, ctx, branch);
	}

	void ThumbFormat15(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		bool type = CHECK_BIT(instr, 11);


		if (type)
			detail::LoadMultiple(instr, bus, ctx, branch);
		else
			detail::StoreMultiple(instr, bus, ctx, branch);
	}

	void ThumbFormat17(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		ctx.EnterException(ExceptionCode::SOFTI, 2);

		branch = true;

		bus->m_time.access = Access::NonSeq;
	}

	void InitThumbJumpTable() {
		for (u8 index = 0; index < 20; index++)
			THUMB_JUMP_TABLE[index] = ThumbUndefined;

		THUMB_JUMP_TABLE[0] = ThumbFormat1;
		THUMB_JUMP_TABLE[1] = ThumbFormat2;
		THUMB_JUMP_TABLE[2] = ThumbFormat3;
		THUMB_JUMP_TABLE[3] = ThumbFormat4;
		THUMB_JUMP_TABLE[4] = ThumbFormat5;
		THUMB_JUMP_TABLE[5] = ThumbFormat6;
		THUMB_JUMP_TABLE[6] = ThumbFormat7;
		THUMB_JUMP_TABLE[7] = ThumbFormat8;
		THUMB_JUMP_TABLE[8] = ThumbFormat9;
		THUMB_JUMP_TABLE[9] = ThumbFormat10;
		THUMB_JUMP_TABLE[10] = ThumbFormat11;
		THUMB_JUMP_TABLE[11] = ThumbFormat12;
		THUMB_JUMP_TABLE[12] = ThumbFormat13;
		THUMB_JUMP_TABLE[13] = ThumbFormat14;
		THUMB_JUMP_TABLE[14] = ThumbFormat15;
		THUMB_JUMP_TABLE[15] = ThumbFormat16;
		THUMB_JUMP_TABLE[16] = ThumbFormat17;
		THUMB_JUMP_TABLE[17] = ThumbFormat18;
		THUMB_JUMP_TABLE[18] = ThumbFormat19;
	}

	THUMBInstructionType DecodeThumb(u16 opcode) {
		for (u8 i = 0; i < 19; i++) {
			u16 masked = opcode & THUMB_MASKS[i];

			if (masked == THUMB_VALUES[i]) {
				if (i == 0 && ((opcode >> 11) & 0x3) == 0x3)
					return THUMBInstructionType::FORMAT_02;
				else if (i == 15 && ((opcode >> 8) & 0xF) == 0xF)
					return THUMBInstructionType::FORMAT_17;

				return (THUMBInstructionType)i;
			}
		}

		return THUMBInstructionType::UNDEFINED;
	}

	void ExecuteThumb(THUMBInstruction instr, memory::Bus* bus, CPUContext& ctx, bool& branch) {
		u16 hash = (instr >> 6) & 0b1111111111;

		THUMBInstructionType type = detail::thumb_lookup_table[hash];

		THUMB_JUMP_TABLE[(u8)type](instr, bus, ctx, branch);
	}
}