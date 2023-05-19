#include "../../memory/Keypad.hpp"

#include "../../memory/InterruptController.hpp"
#include "../../memory/MMIO.hpp"

#include "../../common/BitManip.hpp"

namespace GBA::input {
	using namespace common;

	Keypad::Keypad() :
		m_status{}, m_control{}, m_int_control(nullptr) 
	{
		m_status = 0x3FF;
	}

	void Keypad::SetMMIO(memory::MMIO* mmio) {
		mmio->AddRegister<u16>(KEYPAD_REG_OFFSET, true, false, reinterpret_cast<u8*>(&m_status), 0x0);
		mmio->AddRegister<u16>(KEYPAD_REG_OFFSET + 0x2, true, true, reinterpret_cast<u8*>(&m_control), 0xFFFF);
	}

	void Keypad::KeyPressed(Buttons key_type) {
		m_status &= ~(u16)key_type & 0x3FF;

		RequestInterrupt();
	}

	void Keypad::KeyReleased(Buttons key_type) {
		//m_status = 0x3FF;
		m_status |= (u16)key_type;
	}

	common::u16 Keypad::GetKeyStatus() const {
		return m_status;
	}

	common::u16 Keypad::GetKeyControl() const {
		return m_control;
	}

	void Keypad::SetInterruptController(memory::InterruptController* int_control) {
		m_int_control = int_control;
	}

	void Keypad::RequestInterrupt() {
		if (!CHECK_BIT(m_control, 14))
			return;

		u16 keys = ~m_status & 0x3FF;
		u16 required = m_control & 0x3FF;

		bool do_request = false;

		if (!CHECK_BIT(m_control, 15)) {
			do_request = keys & required;
		}
		else {
			do_request = (keys & required) == required;
		}
	}
}

