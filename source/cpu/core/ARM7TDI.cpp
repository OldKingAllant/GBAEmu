#include "../../../cpu/core/ARM7TDI.hpp"
#include "../../../cpu/arm/ARM_Implementation.hpp"
#include "../../../cpu/thumb/THUMB_Implementation.hpp"

#include <cassert>

#include "../../../memory/InterruptController.hpp"

#include "../../../common/Logger.hpp"

namespace GBA::cpu {

	ARM7TDI::ARM7TDI() :
		m_ctx{}, m_bus{nullptr}, 
		m_int_controller{nullptr}, 
		m_halt{false}, m_cache{},
		m_enable_cache{false},
		m_enable_waitloop_detection{false},
		m_sched{nullptr} {
		m_ctx.m_cpsr.instr_state = InstructionMode::ARM;
		m_ctx.ChangeMode(Mode::SYS);

		m_ctx.m_regs.SetReg(Mode::SWI, 13, 0x03007FE0);
		m_ctx.m_regs.SetReg(Mode::IRQ, 13, 0x03007FA0);
		m_ctx.m_regs.SetReg(Mode::User, 13, 0x03007F00);
	}

	void ARM7TDI::AttachBus(memory::Bus* bus) {
		m_bus = bus;
		m_ctx.m_pipeline.AttachBus(bus);
		m_ctx.m_pipeline.Bubble<InstructionMode::ARM>(0x00);
		m_sched = bus->GetScheduler();
	}

	void ARM7TDI::SkipBios() {
		m_ctx.m_regs.SetReg(15, 0x08000000);
		m_ctx.m_regs.SwitchMode(Mode::User);

		m_ctx.m_pipeline.Bubble<InstructionMode::ARM>(0x08000000);
		m_ctx.m_old_pc = 0x08000000;
	}

	u32 ARM7TDI::GetExceptVector(ExceptionCode const& mode) {
		switch (mode)
		{
		case GBA::cpu::ExceptionCode::RESET:
			return 0x00;
			break;
		case GBA::cpu::ExceptionCode::UNDEF:
			return 0x04;
			break;
		case GBA::cpu::ExceptionCode::SOFTI:
			return 0x08;
			break;
		case GBA::cpu::ExceptionCode::PABRT:
			return 0x0C;
			break;
		case GBA::cpu::ExceptionCode::DABRT:
			return 0x10;
			break;
		case GBA::cpu::ExceptionCode::ADDRE:
			return 0x14;
			break;
		case GBA::cpu::ExceptionCode::IRQ:
			return 0x18;
			break;
		case GBA::cpu::ExceptionCode::FIQ:
			return 0x1C;
			break;
		default:
			break;
		}

		return static_cast<u32>(-1);
	}

	Mode ARM7TDI::GetModeFromExcept(ExceptionCode const& exc) {
		switch (exc)
		{
		case ExceptionCode::RESET:
		case ExceptionCode::SOFTI:
		case ExceptionCode::ADDRE:
			return Mode::SWI;

		case ExceptionCode::UNDEF:
			return Mode::UND;

		case ExceptionCode::PABRT:
		case ExceptionCode::DABRT:
			return Mode::ABRT;

		case ExceptionCode::IRQ:
			return Mode::IRQ;

		case ExceptionCode::FIQ:
			return Mode::FIQ;

		default:
			break;
		}

		return static_cast<Mode>(-1);
	}

	void CPUContext::ChangeMode(Mode new_mode) {
		m_regs.SwitchMode(new_mode);
		u8 mode_id = GetModeFromID(new_mode);
		m_spsr[mode_id - 1] = m_cpsr;
		m_cpsr.mode = new_mode;
	}

	void CPUContext::EnterException(ExceptionCode exc, u8 pc_offset) {
		Mode mode = ARM7TDI::GetModeFromExcept(exc);

		m_regs.SwitchMode(mode);

		u8 mode_id = GetModeFromID(mode);

		m_spsr[mode_id - 1] = m_cpsr;

		m_cpsr.instr_state = InstructionMode::ARM;
		m_cpsr.irq_disable = true;

		if(exc == ExceptionCode::RESET ||
			exc == ExceptionCode::FIQ)
			m_cpsr.fiq_disable = true;

		u32 exc_vector = ARM7TDI::GetExceptVector(exc);

		m_old_pc = m_regs.GetReg(15);

		m_regs.SetReg(14, m_regs.GetReg(15) + pc_offset);
		m_regs.SetReg(15, exc_vector);

		m_cpsr.mode = mode;
	}
	
	void CPUContext::RestorePreviousMode(u32 old_pc) {
		assert(m_cpsr.mode != Mode::User
			&& m_cpsr.mode != Mode::SYS);
			
		u8 curr_mode_id = GetModeFromID(
			m_cpsr.mode
		);

		Mode old_mode = m_spsr[curr_mode_id - 1]
			.mode;

		m_regs.SwitchMode(old_mode);
		m_regs.SetReg(15, old_pc);

		m_cpsr = m_spsr[curr_mode_id - 1];
	}

	void ARM7TDI::SetInterruptControl(memory::InterruptController* control) {
		m_int_controller = control;
	}

	bool ARM7TDI::CheckIRQ() {
		if (m_int_controller->GetLineStatus())
			return false; //So an interrupt is triggered
		//only once and not every time we check 
		//IE and IF registers

		bool cpsr_ime = !m_ctx.m_cpsr.irq_disable;
		bool ime = m_int_controller->GetIME();
		u16 ie = m_int_controller->GetIE();
		u16 _if = m_int_controller->GetIF();

		if (cpsr_ime && ime && (ie & _if)) {
			m_int_controller->ResetLineStatus();
			return true;
		}

		return false;
	}

	void ARM7TDI::EvaluateHaltState() {
		u16 ie  = m_int_controller->GetIE();
		u16 _if = m_int_controller->GetIF();

		if (ie & _if) {
			m_halt = false;
			m_bus->ResetHalt();
		}
	}

	void ARM7TDI::Step() {
		//Check if IRQ occurred
		//Check halt status
		if (m_halt) [[unlikely]] {
			u16 ie = m_int_controller->GetIE();
			u16 _if = m_int_controller->GetIF();

			if (ie & _if) {
				m_halt = false;
				m_bus->ResetHalt();
			}
			else {
				m_bus->InternalCycles(1);
				return;
			}
		}

		if (CheckIRQ()) {
			m_ctx.EnterException(ExceptionCode::IRQ, 4);

			m_ctx.m_pipeline.Bubble<InstructionMode::ARM>(
				m_ctx.m_regs.GetReg(15)
			);
		}

		if (m_enable_cache) {
			StepCached();
		}
		else {
			StepNoCache();
		}

		m_ctx.m_old_pc = m_ctx.m_regs.GetReg(15);
	}

	void ARM7TDI::StepCached() {
		u32 curr_pc = m_ctx.m_regs.GetReg(15);
		auto block = m_cache.GetBlock(curr_pc);

		if (block == nullptr) {
			//No cache exists, and cannot be created
			//for the current region
			(void)StepNoCache();
		}
		else {
			if (*block == nullptr) {
				//No cache, but can be made
				RunMakeCache();
			}
			else {
				//Has cache

				//Get real block pointer
				auto& real_block = *block;
				//List of instructions in the block
				auto const& instr_list = real_block->instructions;
				//Save the first instruction in case
				//there is a loop
				auto first_instr = instr_list.begin();
				auto curr_instr  = first_instr;
				auto end         = instr_list.end();
				auto instr_set   = real_block->instr_set;

				auto prev_mode = m_ctx.m_cpsr.instr_state;

				//In theory, the same code block could be executed
				//both in arm and thumb mode, but it is so
				//unlikely that I simply consider it to be
				//an error
				if (prev_mode != instr_set) [[unlikely]] {
					fmt::println("[INTERPRETER] INSTR SET MISMATCH, block at {:#010x}",
						curr_pc);
					error::DebugBreak();
				}

				//Save a reference to this flag, which tells whether
				//an event changed the CPU state during the execution
				//of an instruction
				bool& event_changed_cpu = m_sched->did_influence_cpu;

				//Increment of PC after an instruction
				i32 pc_step {};
				u32 cycles  {};
				//Save in case the block is a loop
				u32 base_pc {curr_pc};

				bool did_branch_final = false;

				//fmt::println("[INTERPRETER] Block at {:#010x}", curr_pc);

				while (curr_instr != end) {
					bool has_branched = false;
					
					if (instr_set == InstructionMode::ARM) {
						auto instr = arm::ARMInstruction(curr_instr->orig_instruction);

						//We need to check the condition here
						if (m_ctx.m_cpsr.CheckCondition(instr.condition)) 
						{
							reinterpret_cast<arm::ArmExecutor>(curr_instr->arm_func)
								(instr, m_ctx, m_bus, has_branched);
						}
						else {
							m_bus->m_time.access = Access::Seq;
						}

						pc_step = 4;

						//Compute cycles due to fetch, without actually
						//fetching the instruction
						cycles = m_bus->m_time.GetCyleCountForCachedInterpreter<u32>(
							curr_pc
						);
					}
					else {
						reinterpret_cast<thumb::ThumbFunc>(curr_instr->thumb_func)
							(u16(curr_instr->orig_instruction), m_bus, m_ctx, has_branched);
						pc_step = 2;
						cycles = m_bus->m_time.GetCyleCountForCachedInterpreter<u16>(
							curr_pc
						);
					}

					//Actually step the scheduler
					m_sched->Advance(cycles);

					if (has_branched) {
						did_branch_final = true;
						if (m_ctx.m_regs.GetReg(15) != base_pc) {
							//For sure the entire block is not 
							//a loop. Since we do not want
							//to jump in the middle of a block,
							//also loops that are entirely inside
							//the block are not handled
							break;
						}
						//Something changed, we need to break anyway
						if (event_changed_cpu || m_halt || m_bus->GetActiveDma() != memory::Bus::INVALID_DMA)
							break;
						if (*block == nullptr) [[unlikely]]
							break;

						if (m_enable_waitloop_detection) {
							if (real_block->waitloop_evaluation == WaitloopState::NOT_EVALUATED) {
								EvaluateLoop(*real_block);
							}
							else if (real_block->waitloop_evaluation == WaitloopState::WAITLOOP) {
								if (RunWaitLoop(*real_block))
									break;
							}
						}

						//"No jump occurred" 
						did_branch_final = false;
						//Reset instruction and PC
						curr_instr = first_instr;
						curr_pc = base_pc;

						continue;
						//Ideally here we can infer if a loop is
						//a waitloop (e.g. waiting for an hardware
						//event) and try to skip it (as if
						//the CPU was halted)
					}

					//Get the new instruction set.
					//If it changed, we need to break
					auto new_mode = m_ctx.m_cpsr.instr_state;

					//Update the PC, else PC-relative
					//computations will not work
					curr_pc += pc_step;
					m_ctx.m_regs.AddOffset(15, pc_step);

					//Important for emulating accurate
					//BIOS accesses
					m_ctx.m_old_pc = curr_pc;

					if (event_changed_cpu || m_halt || 
						m_bus->GetActiveDma() != memory::Bus::INVALID_DMA ||
						new_mode != prev_mode)
						break;

					if (*block == nullptr) [[unlikely]] {
						//A write near pc happened
						//and the current block
						//has been invalidated
						break;
					}

					++curr_instr;
				}

				u32 new_pc = m_ctx.m_regs.GetReg(15);

				//Pipeline emulation changes depending on whether
				//there was a branch or not
				if (did_branch_final) {
					//Normal branch behaviour: bubble + scheduler
					if (m_ctx.m_cpsr.instr_state == InstructionMode::ARM) {
						new_pc &= ~3;
						m_ctx.m_pipeline.Bubble<InstructionMode::ARM>(new_pc);
					}
					else {
						new_pc &= ~1;
						m_ctx.m_pipeline.Bubble<InstructionMode::THUMB>(new_pc);
					}
					m_ctx.m_regs.SetReg(15, new_pc);

					using memory::MEMORY_RANGE;

					auto old_region = MEMORY_RANGE(m_ctx.m_old_pc >> 24);
					auto new_region = MEMORY_RANGE(new_pc         >> 24);

					if (old_region == MEMORY_RANGE::BIOS &&
						new_region != MEMORY_RANGE::BIOS) {
						//Update BIOS latch value, else
						//BIOS access will not be 100%
						//accurate, which might break
						//games that depend on it
						//(e.g. in Minish Cap, Link cannot
						//roll if this is not emulated
						//correctly)
						switch (m_ctx.m_old_pc)
						{
						case memory::Bus::BIOS_END_RES_PC:
							m_bus->LoadBiosResetOpcode();
							break;
						case memory::Bus::BIOS_MID_IRQ_PC:
							m_bus->LoadBiosMidIRQOpcode();
							break;
						case memory::Bus::BIOS_END_IRQ_PC:
							m_bus->LoadBiosEndIRQOpcode();
							break;
						case memory::Bus::BIOS_END_SWI_PC:
							m_bus->LoadBiosSWIOpcode();
							break;
						default:
							break;
						}
					}
				}
				else {
					//No bubble necessary. In theory pipeline has already been emulated
					//during block execution, but this is not true, since we are only
					//stepping the scheduler and not actually fetching opcodes, so
					//we actually fetch the instructions, without stepping the
					//scheduler
					if (m_ctx.m_cpsr.instr_state == InstructionMode::ARM) {
						m_ctx.m_pipeline.Bubble<InstructionMode::ARM, false>(new_pc);
					}
					else {
						m_ctx.m_pipeline.Bubble<InstructionMode::THUMB, false>(new_pc);
					}
				}
				

				event_changed_cpu = false;

				//No need to update old PC, since the callee
				//will do it
			}
		}
	}

	std::pair<bool, u32> ARM7TDI::StepNoCache() {
		bool branch{false};
		u32  executed_opcode{};

		if (m_ctx.m_cpsr.instr_state == InstructionMode::ARM) {
			u32 opcode = m_ctx.m_pipeline.Pop<InstructionMode::ARM>();
			arm::ExecuteArm(opcode, m_ctx, m_bus, branch);
			m_ctx.m_pipeline.Fetch<InstructionMode::ARM>();

			executed_opcode = opcode;
		}
		else {
			u16 opcode = m_ctx.m_pipeline.Pop<InstructionMode::THUMB>();
			thumb::ExecuteThumb(opcode, m_bus, m_ctx, branch);
			m_ctx.m_pipeline.Fetch<InstructionMode::THUMB>();

			executed_opcode = u32(opcode);
		}

		if (branch) {
			u32 pc = m_ctx.m_regs.GetReg(15);

			if (m_ctx.m_cpsr.instr_state == InstructionMode::ARM) {
				pc &= ~3;
				m_ctx.m_pipeline.Bubble<InstructionMode::ARM>(pc);
			}
			else {
				pc &= ~1;
				m_ctx.m_pipeline.Bubble<InstructionMode::THUMB>(pc);
			}

			m_ctx.m_regs.SetReg(15, pc);
		}
		else {
			m_ctx.m_regs.AddOffset(15, m_ctx.m_cpsr.instr_state == InstructionMode::ARM ?
				0x4 : 0x2);
		}

		return {branch, executed_opcode};
	}

	void ARM7TDI::RunMakeCache() {
		u32 base_pc = m_ctx.m_regs.GetReg(15);
		u32 curr_pc = base_pc;
		auto page = m_cache.GetPageFromAddress(base_pc);

		Block instr_block{};
		instr_block.instr_set = m_ctx.m_cpsr.instr_state;

		u32 curr_block_len = {};
		auto max_block_len = m_cache.GetBlockLen();

		m_sched->did_influence_cpu = false;

		while (true) {
			auto prev_instr_mode = m_ctx.m_cpsr.instr_state;
			auto state = StepNoCache();
			auto& [has_branched, opcode] = state;
			auto new_instr_mode = m_ctx.m_cpsr.instr_state;

			BlockEntry instr_entry{};
			instr_entry.orig_instruction = opcode;

			if (prev_instr_mode == InstructionMode::ARM) {
				auto handler = arm::GetArmHandler(arm::ARMInstruction(opcode));
				instr_entry.arm_func = std::bit_cast<void*>(handler);
				curr_pc += 4;
			}
			else {
				auto handler = thumb::GetThumbHandler(thumb::THUMBInstruction(opcode));
				instr_entry.thumb_func = std::bit_cast<void*>(handler);
				curr_pc += 2;
			}
			
			bool event_influenced_cpu = m_sched->did_influence_cpu;

			if (event_influenced_cpu) {
				m_sched->did_influence_cpu = false;
				return;
			}

			instr_block.instructions.push_back(instr_entry);
			curr_block_len += prev_instr_mode == InstructionMode::ARM ? 4 : 2;

			auto new_page = m_cache.GetPageFromAddress(curr_pc);

			if (has_branched || curr_block_len >= max_block_len ||
				new_page != page || prev_instr_mode != new_instr_mode ||
				m_halt || m_bus->GetActiveDma() != memory::Bus::INVALID_DMA) {
				break;
			}
		}

		m_sched->did_influence_cpu = false;
		m_cache.AddBlock(base_pc, std::move(instr_block));
	}

	namespace waitloop {
		static bool IsValidLoad(u16 instruction, CPUContext const& ctx, u32& poll_address) {
			auto first_instruction_ty = thumb::DecodeThumb(instruction);

			//Check if it is a valid load/store instruction
			//(non PC-relative or SP-relative)
			if (u8(first_instruction_ty) < u8(thumb::THUMBInstructionType::FORMAT_07) ||
				u8(first_instruction_ty) > u8(thumb::THUMBInstructionType::FORMAT_10)) {
				return false;
			}

			using thumb::THUMBInstructionType;

			u32 opcode {};

			switch (first_instruction_ty)
			{
			case THUMBInstructionType::FORMAT_07:
				opcode = (instruction >> 10) & 3;
				if (opcode == 0 || opcode == 1) {
					//It is a store instruction
					return false;
				}

				{
					u8 offset_reg = (instruction >> 6) & 0x7;
					u8 base_reg   = (instruction >> 3) & 0x7;
					poll_address  = ctx.m_regs.GetReg(base_reg)
						          + ctx.m_regs.GetReg(offset_reg);
				}
				break;
			case THUMBInstructionType::FORMAT_08:
				opcode = (instruction >> 10) & 3;
				if (opcode != 2) {
					//It is a sign-extend load or a store
					return false;
				}

				{
					u8 offset_reg = (instruction >> 6) & 0x7;
					u8 base_reg   = (instruction >> 3) & 0x7;
					poll_address  = ctx.m_regs.GetReg(base_reg)
						          + ctx.m_regs.GetReg(offset_reg);
				}
				break;
			case THUMBInstructionType::FORMAT_09:
				opcode = (instruction >> 11) & 3;
				if (opcode == 0 || opcode == 2) {
					//It is a store
					return false;
				}

				{
					u32 offset    = (instruction >> 6) & 0x1F;
					u8 base_reg   = (instruction >> 3) & 0x7;
					u32 base_addr = ctx.m_regs.GetReg(base_reg);

					if (opcode == 1) {
						poll_address = base_addr + (offset << 2);
					}
					else {
						poll_address = base_addr + offset;
					}
				}
				break;
			case THUMBInstructionType::FORMAT_10:
				opcode = (instruction >> 11) & 1;
				if (opcode == 0) {
					return false;
				}

				{
					u32 offset   = ((instruction >> 6) & 0x1F) * 2;
					u8 base_reg  = (instruction >> 3) & 0x7;
					poll_address = ctx.m_regs.GetReg(base_reg) + offset;
				}
				break;
			default:
				error::Unreachable();
				break;
			}

			return true;
		}

		static bool IsValidCompare(u16 instruction) {
			auto instruction_ty = thumb::DecodeThumb(instruction);

			if (u8(instruction_ty) < u8(thumb::THUMBInstructionType::FORMAT_03) ||
				u8(instruction_ty) > u8(thumb::THUMBInstructionType::FORMAT_05)) {
				return false;
			}

			using thumb::THUMBInstructionType;

			u32 opcode{};

			switch (instruction_ty)
			{
			case THUMBInstructionType::FORMAT_03:
				opcode = (instruction >> 11) & 3;
				if (opcode != 1) {
					return false;
				}
				break;
			case THUMBInstructionType::FORMAT_04:
				opcode = (instruction >> 6) & 0xF;
				if (opcode != 0xA && opcode != 0XB) {
					return false;
				}
				break;
			case THUMBInstructionType::FORMAT_05:
				opcode = (instruction >> 8) & 0x3;
				if (opcode != 0x1) {
					return false;
				}
				break;
			default:
				error::Unreachable();
				break;
			}

			return true;
		}

		static bool IsAllowed(u16 instruction, u8& rd) {
			auto instruction_ty = thumb::DecodeThumb(instruction);

			if (u8(instruction_ty) > u8(thumb::THUMBInstructionType::FORMAT_05)) {
				return false;
			}

			using thumb::THUMBInstructionType;

			switch (instruction_ty)
			{
			case THUMBInstructionType::FORMAT_01:
			case THUMBInstructionType::FORMAT_02:
			case THUMBInstructionType::FORMAT_04:
				rd = (instruction & 0x7);
				break;
			case THUMBInstructionType::FORMAT_03:
				rd = (instruction >> 8) & 0x7;
				break;
			case THUMBInstructionType::FORMAT_05:
				rd = (instruction & 0x7) | 
					 ((instruction >> 4) & 0b1000);
				break;
			default:
				error::Unreachable();
				break;
			}

			return true;
		}
	}

	void ARM7TDI::EvaluateLoop(Block& loop) {
		if (loop.instr_set == InstructionMode::ARM) {
			loop.waitloop_evaluation = WaitloopState::NOT_WAITLOOP;
			return;
		}

		{
			const auto loop_size = loop.instructions.size();
			if (loop_size < 3 || loop_size > 6) {
				loop.waitloop_evaluation = WaitloopState::NOT_WAITLOOP;
				return;
			}
		}

		//Try to evaluate a fixed number of possible waitloops
		//First instruction should be a memory load
		//Last instruction is for sure a backwards 
		//conditional jump, given that this function
		//has been called.

		//After the load, only one register should change
		//value. The last instruction before the jump should be
		//a cmp or cmn

		//Acceptable instructions in-between are logical/ALU
		//operations

		//For load, we have the following formats:
		//7, 8, 9, 10

		//For CMP/CMN we have
		//3, 4, 5

		u32 poll_address{ 0xdeadbeef };

		if (!waitloop::IsValidLoad(u16(loop.instructions[0].orig_instruction), m_ctx,
			poll_address)) {
			loop.waitloop_evaluation = WaitloopState::NOT_WAITLOOP;
			return;
		}

		constexpr u32 IO_DISPSTAT = 0x04000004;
		constexpr u32 IO_VCOUNT   = 0x04000006;
		constexpr u32 IO_IF       = 0x04000202;

		using memory::MEMORY_RANGE;

		auto poll_region = MEMORY_RANGE(poll_address >> 24);

		switch (poll_region)
		{
		case MEMORY_RANGE::EWRAM:
		case MEMORY_RANGE::IWRAM:
			break;
		case MEMORY_RANGE::IO:
		{
			switch (poll_address)
			{
			case IO_DISPSTAT:
			case IO_VCOUNT:
			case IO_IF:
				break;
			default:
				loop.waitloop_evaluation = WaitloopState::NOT_WAITLOOP;
				return;
			}
		}
			break;
		default:
			loop.waitloop_evaluation = WaitloopState::NOT_WAITLOOP;
			return;
		}

		auto last_instruction = u16((loop.instructions.rbegin() + 1)->orig_instruction);

		if (!waitloop::IsValidCompare(last_instruction)) {
			loop.waitloop_evaluation = WaitloopState::NOT_WAITLOOP;
			return;
		}

		loop.poll_address = poll_address;

		fmt::println("[INTERPRETER] Found possible waitloop at {:#010x}, poll address = {:#010x}",
			loop.absolute_address, poll_address);

		if (loop.instructions.size() == 3) {
			fmt::println("[INTERPRETER] Marking block at {:#010x} as waitloop",
				loop.absolute_address);
			loop.waitloop_evaluation = WaitloopState::WAITLOOP;
			return;
		}

		//Now we need to verify that all instructions in between
		//are allowed

		//Only allowed formats:
		//[1, 5]

		auto first_iter = loop.instructions.begin() + 1;
		auto last_iter  = (loop.instructions.rbegin() + 2).base();

		u8 modified_reg{};
		if (!waitloop::IsAllowed(u16(first_iter->orig_instruction), modified_reg)) {
			loop.waitloop_evaluation = WaitloopState::NOT_WAITLOOP;
			fmt::println("[INTERPRETER] Waitloop rejected, unsupported format");
			return;
		}
		++first_iter;

		while (first_iter != last_iter) {
			u8 mod_reg2{};
			if (!waitloop::IsAllowed(u16(first_iter->orig_instruction), mod_reg2)) {
				loop.waitloop_evaluation = WaitloopState::NOT_WAITLOOP;
				fmt::println("[INTERPRETER] Waitloop rejected, unsupported format");
				return;
			}

			if (modified_reg != mod_reg2) {
				loop.waitloop_evaluation = WaitloopState::NOT_WAITLOOP;
				fmt::println("[INTERPRETER] Waitloop rejected, modifies more than one register");
				return;
			}

			++first_iter;
		}

		fmt::println("[INTERPRETER] Marking block at {:#010x} as waitloop",
			loop.absolute_address);

		loop.waitloop_evaluation = WaitloopState::WAITLOOP;
	}

	bool ARM7TDI::RunWaitLoop(Block& loop)
	{
		u32 poll_address{ 0xdeadbeef };

		auto valid = waitloop::IsValidLoad(u16(loop.instructions[0].orig_instruction), m_ctx,
			poll_address);

		if (!valid) [[unlikely]] {
			fmt::println("[INTERPRETER] \"Impossible\" happened, waitloop does not start with a load instruction");
			error::DebugBreak();
		}

		if (loop.poll_address != poll_address) {
			fmt::println("[INTERPRETER] Waitloop at {:#010x}, poll address changed",
				loop.absolute_address);
			fmt::println("              PREV = {:#010x}, NEW = {:#010x}",
				loop.poll_address, poll_address);
			fmt::println("              Marking as non-waitloop");
			loop.waitloop_evaluation = WaitloopState::NOT_WAITLOOP;
			return false;
		}

		bool do_quit = false;

		do {
			(void)m_sched->NextEvent();
			do_quit = m_sched->did_influence_cpu;
		} while (!do_quit);

		m_sched->did_influence_cpu = false;

		return true;
	}
}