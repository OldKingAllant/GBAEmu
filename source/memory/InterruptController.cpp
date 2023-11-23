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

				//common::u32 to_xor = ((u16)value << (8 * shift_amount));

				u8 reg_val = m_registers[0x2 + shift_amount];

				for (u8 bit = 0; bit < 8; bit++) {
					if (CHECK_BIT(value, bit) && CHECK_BIT(reg_val, bit))
						reg_val = CLEAR_BIT(reg_val, bit);
				}

				m_registers[0x2 + shift_amount] = reg_val;
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
		return m_irq_line;
	}

	void InterruptController::ResetLineStatus() {
		m_irq_line = true;
	}
}