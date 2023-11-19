#include "../../../cpu/core/ARM7TDI.hpp"
#include "../../../cpu/arm/ARM_Implementation.hpp"
#include "../../../cpu/thumb/THUMB_Implementation.hpp"

#include <cassert>

#include "../../../memory/InterruptController.hpp"

namespace GBA::cpu {

	ARM7TDI::ARM7TDI() :
		m_ctx{}, m_bus(nullptr), m_int_controller(nullptr), m_halt(false) {
		m_ctx.m_cpsr.instr_state = InstructionMode::ARM;
		m_ctx.m_cpsr.mode = Mode::User;

		m_ctx.m_regs.SetReg(Mode::SWI, 13, 0x03007FE0);
		m_ctx.m_regs.SetReg(Mode::IRQ, 13, 0x03007FA0);
		m_ctx.m_regs.SetReg(Mode::User, 13, 0x03007F00);

		thumb::InitThumbJumpTable();
	}

	void ARM7TDI::AttachBus(memory::Bus* bus) {
		m_bus = bus;
		m_ctx.m_pipeline.AttachBus(bus);
		m_ctx.m_pipeline.Bubble<InstructionMode::ARM>(0x00);
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

	u8 ARM7TDI::Step() {
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
				return 0;
			}
		}

		if (CheckIRQ()) {
			m_ctx.EnterException(ExceptionCode::IRQ, 4);

			m_ctx.m_pipeline.Bubble<InstructionMode::ARM>(
				m_ctx.m_regs.GetReg(15)
			);
		}

		bool branch = false;

		//Let the executed instruction decide if
		//it should change the access to nonseq
		//m_bus->m_time.access = memory::Access::NonSeq;

		if (m_ctx.m_cpsr.instr_state == InstructionMode::ARM) {
			u32 opcode = m_ctx.m_pipeline.Pop<InstructionMode::ARM>();

			arm::ExecuteArm(opcode, m_ctx, m_bus, branch);

			m_ctx.m_pipeline.Fetch<InstructionMode::ARM>();
		}
		else {
			u16 opcode = m_ctx.m_pipeline.Pop<InstructionMode::THUMB>();

			thumb::ExecuteThumb(opcode, m_bus, m_ctx, branch);

			m_ctx.m_pipeline.Fetch<InstructionMode::THUMB>();
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

			m_bus->StopPrefetch();
		}
		else {
			if (m_ctx.m_cpsr.instr_state == InstructionMode::ARM) {
				m_ctx.m_regs.AddOffset(15, 0x4);
			}
			else {
				m_ctx.m_regs.AddOffset(15, 0x2);
			}
		}

		m_ctx.m_old_pc = m_ctx.m_regs.GetReg(15);

		return 0;
	}
}