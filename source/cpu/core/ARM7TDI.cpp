#include "../../../cpu/core/ARM7TDI.hpp"
#include "../../../cpu/arm/ARM_Implementation.h"

#include <cassert>

namespace GBA::cpu {

	ARM7TDI::ARM7TDI() :
		m_ctx{} {
		m_ctx.m_cpsr.instr_state = InstructionMode::ARM;
		m_ctx.m_cpsr.mode = Mode::User;

		m_ctx.m_regs.SetReg(Mode::SWI, 13, 0x03007FE0);
		m_ctx.m_regs.SetReg(Mode::IRQ, 13, 0x03007FA0);
		m_ctx.m_regs.SetReg(Mode::User, 13, 0x03007F00);

		m_ctx.m_pipeline.Bubble<InstructionMode::ARM>(0x00);
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

	void CPUContext::EnterException(ExceptionCode exc) {
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

		m_regs.SetReg(14, 0/*m_regs.GetReg(15)*/ /*Save ret. address*/);
		m_regs.SetReg(15, exc_vector);

		m_pipeline.Bubble<InstructionMode::ARM>(exc_vector);

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

	u8 ARM7TDI::Step() {
		//Check if IRQ occurred
		//Check halt status

		bool branch = false;

		if (m_ctx.m_cpsr.instr_state == InstructionMode::ARM) {
			u32 opcode = m_ctx.m_pipeline.Pop<InstructionMode::ARM>();

			m_ctx.m_pipeline.Fetch<InstructionMode::ARM>();

			arm::ExecuteArm(opcode, m_ctx, m_bus, branch);
		}
		else {
			u16 opcode = m_ctx.m_pipeline.Pop<InstructionMode::THUMB>();

			m_ctx.m_pipeline.Fetch<InstructionMode::THUMB>();

			//Execute thumb instr.
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
			if (m_ctx.m_cpsr.instr_state == InstructionMode::ARM)
				m_ctx.m_regs.AddOffset(15, 0x4);
			else 
				m_ctx.m_regs.AddOffset(15, 0x2);
		}

		return 0;
	}
}