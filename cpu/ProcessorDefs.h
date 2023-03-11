#ifndef PROC_DEFS
#define PROC_DEFS

#include "../common/Defs.hpp"

namespace GBA::cpu {
	using namespace common;
	/*
	* Processor modes.
	* They define the current
	* register bank.
	*/
	enum class Mode : u8 {
		User = 0x10,
		FIQ = 0x11,
		IRQ = 0x12,
		SWI = 0x13,
		ABRT = 0x17,
		UND = 0x1B,
		SYS = 0x1F
	};

	static u8 GetModeFromID(Mode mode) {
		switch (mode)
		{
		case GBA::cpu::Mode::User:
		case GBA::cpu::Mode::SYS:
			return 0x00;
			break;
		case GBA::cpu::Mode::FIQ:
			return 0x01;
			break;
		case GBA::cpu::Mode::IRQ:
			return 0x02;
			break;
		case GBA::cpu::Mode::SWI:
			return 0x03;
			break;
		case GBA::cpu::Mode::ABRT:
			return 0x04;
			break;
		case GBA::cpu::Mode::UND:
			return 0x05;
			break;
		default:
			break;
		}

		return static_cast<u8>(-1);
	}
}

#endif
