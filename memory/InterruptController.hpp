#pragma once

#include "../common/Defs.hpp"

namespace GBA::memory {
	class MMIO;

	static constexpr common::u16 INTERRUPT_REG_BASE = 0x200;

	enum class InterruptType {
		VBLANK = (1 << 0),
		HBLANK = (1 << 1),
		VCOUNT = (1 << 2),
		TIMER0 = (1 << 3),
		TIMER1 = (1 << 4),
		TIMER2 = (1 << 5),
		TIMER3 = (1 << 6), 
		SERIAL = (1 << 7),
		DMA0 = (1 << 8),
		DMA1 = (1 << 9),
		DMA2 = (1 << 10),
		DMA3 = (1 << 11),
		KEYPAD = (1 << 12),
		GPAK = (1 << 13)
	};

	class InterruptController {
	public :
		InterruptController(MMIO* mmio);

		bool GetIME() const;
		common::u16 GetIE() const;
		common::u16 GetIF() const;

		bool GetLineStatus();
		void ResetLineStatus();

		void RequestInterrupt(InterruptType type);

	private :
		common::u8 m_registers[0xC];

		bool m_irq_line;
	};
}