#include "../../memory/InterruptController.hpp"

#include "../../memory/MMIO.hpp"

namespace GBA::memory {
	using namespace common;

	InterruptController::InterruptController(MMIO* mmio) :
		m_registers{}, m_irq_line(true)
	{
		mmio->AddRegister<u16>(INTERRUPT_REG_BASE, true, true, &m_registers[0], 0xFFFF);
		mmio->AddRegister<u32>(INTERRUPT_REG_BASE + 0x8, true, true, &m_registers[0x8], 0xFFFFFFFF);
		mmio->AddRegister<u16>(INTERRUPT_REG_BASE + 0x2, true, true, &m_registers[0x2], 0xFFFF, 
			[this](common::u8 value, common::u16 offset) {
				common::u8 shift_amount = offset % 2;

				common::u32 to_xor = (value << (8 * shift_amount));

				m_registers[0x2 + shift_amount] ^= to_xor;
			});
	}

	bool InterruptController::GetIME() const {
		return m_registers[0x8] & 1;
	}

	common::u16 InterruptController::GetIE() const {
		return *reinterpret_cast<u16 const*>(m_registers);
	}

	common::u16 InterruptController::GetIF() const {
		return *reinterpret_cast<u16 const*>(m_registers + 0x2);
	}

	void InterruptController::RequestInterrupt(InterruptType type) {
		*reinterpret_cast<u16*>(m_registers + 0x2) |= (u16)type;
		m_irq_line = false;
	}

	bool InterruptController::GetLineStatus() {
		bool status = m_irq_line;

		m_irq_line = true;

		return status;
	}
}