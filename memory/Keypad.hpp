#pragma once

#include "../common/Defs.hpp"

namespace GBA::memory {
	class MMIO;
	class InterruptController;
}

namespace GBA::input {
	enum class Buttons {
		BUTTON_A = 0x1,
		BUTTON_B = 0x2,
		BUTTON_SELECT = 0x4,
		BUTTON_START = 0x8,
		BUTTON_RIGHT = 0x10,
		BUTTON_LEFT = 0x20,
		BUTTON_UP = 0x40,
		BUTTON_DOWN = 0x80,
		BUTTON_R = 0x100,
		BUTTON_L = 0x200
	};

	class Keypad {
	public :
		Keypad();
		void KeyPressed(Buttons key_type);
		void KeyReleased(Buttons key_type);

		common::u16 GetKeyStatus() const;
		common::u16 GetKeyControl() const;

		void SetInterruptController(memory::InterruptController* int_control);
		void SetMMIO(memory::MMIO* mmio);

		static constexpr common::u32 KEYPAD_REG_OFFSET = 0x130;

	private :
		void RequestInterrupt();

	private :
		common::u16 m_status;
		common::u16 m_control;

		memory::InterruptController* m_int_control;
	};
}