#include "../../../cpu/arm/ARM_Implementation.h"
#include "../../../cpu/core/CPUContext.hpp"
#include "../../../memory/Bus.hpp"

namespace GBA::cpu::arm {
	ARMInstructionType DecodeArm(u32 opcode) {
		u16 bits = (((opcode >> 20) & 0xFF) << 4) | ((opcode >> 4) & 0xF);
		u32 masked = bits & ARMInstructionMask::BRANCH;

		if (masked == ARMInstructionCode::BRANCH)
			return ARMInstructionType::BRANCH;

		masked = bits & ARMInstructionMask::BLOCK_TRANSFER;

		if (masked == ARMInstructionCode::BLOCK_TRANSFER)
			return ARMInstructionType::BLOCK_DATA_TRANSFER;

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
		
	}

	void BlockDataTransfer(ARMBlockTransfer instr, CPUContext& ctx, memory::Bus* bus, bool& branch) {
		//The register list is 16 bits long, and 
		//when a bit is 1 the corresponding register 
		//r[n] is in the list

		//Get base register
		u32 base = ctx.m_regs.GetReg(instr.base_reg);
		u32 original_base = base;

		if (!instr.increment) {
			//Registers are processed in increasing
			//addresses, which means that if we
			//are decrementing the address after
			//each load/store, we should start
			//from the lowest address and then increment
			u32 rlist = instr.rlist;

			while (rlist) {
				if (rlist & 1)
					base -= 4;

				rlist >>= 1;
			}
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
		case ARMInstructionType::UNDEFINED:
			break;
		default:
			break;
		}
	}
}