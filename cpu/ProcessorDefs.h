#ifndef PROC_DEFS
#define PROC_DEFS

#include "../common/Defs.hpp"

#include <type_traits>
#include <string_view>

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

	static std::string_view GetStringFromMode(Mode mode) {
		switch (mode) {
		case Mode::User:
			return "USR";

		case Mode::SYS:
			return "SYS";

		case GBA::cpu::Mode::FIQ:
			return "FIQ";
			
		case GBA::cpu::Mode::IRQ:
			return "IRQ";
			
		case GBA::cpu::Mode::SWI:
			return "SWI";
			
		case GBA::cpu::Mode::ABRT:
			return "ABRT";
			
		case GBA::cpu::Mode::UND:
			return "UND";
		}

		return "";
	}

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

	static Mode GetIDFromMode(u8 mode) {
		switch (mode)
		{
		case 0x0:
			return Mode::User;

		case 0x1:
			return Mode::FIQ;

		case 0x2:
			return Mode::IRQ;

		case 0x3:
			return Mode::SWI;
			
		case 0x4:
			return Mode::ABRT;
			
		case 0x5:
			return Mode::UND;

		default:
			return (Mode)0;
			break;
		}
	}

	enum class ExceptionCode {
		RESET,
		UNDEF,
		SOFTI,
		PABRT,
		DABRT,
		ADDRE,
		IRQ,
		FIQ
	};

	/*
	* Opcode type and len.
	*/
	enum class InstructionMode : bool {
		ARM = 0,
		THUMB = 1
	};

	static std::string_view GetStringFromState(InstructionMode mode) {
		return mode == InstructionMode::ARM ? "ARM" : "THUMB";
	}
}

#endif
