#pragma once

#include "Cheats.hpp"

namespace GBA::cheats {
	enum class AR_Opcodes {
		HOOK,
		ID_CODE,
		SEEDS,
		WRITE_8,
		WRITE_16,
		WRITE_32
	};

	enum class AR_OpcodeMatch {
		HOOK     = 0xC4,
		WRITE_8  = 0x00,
		WRITE_16 = 0x02,
		WRITE_32 = 0x04,
		IF_COND  = 0x38 
	};

	enum class AR_OpcodeMatchSpecial : uint32_t {
		ID_CODE = 0x001DC0DE,
		SEEDS   = 0xDEADFACE,
		SPECIAL = 0x00000000
	};

	enum class AR_Cond {
		EQ = 0x08,
		NE = 0x10,
		LT = 0x18,
		GT = 0x20,
		LTU = 0x28,
		GTU = 0x30,
		AND = 0x38
	};

	enum class AR_CondSize {
		SIZE_8  = 0x00,
		SIZE_16 = 0x02,
		SIZE_32 = 0x04,
		FALSE   = 0x06
	};

	enum class AR_CondAction {
		NEXT_1 = 0x00,
		NEXT_2 = 0x40,
		BLOCK  = 0x80,
		OFF    = 0xC0
	};

	constexpr uint32_t AR_COND_MASK        = 0b0011'1000;
	constexpr uint32_t AR_COND_SIZE_MASK   = 0b0000'0110;
	constexpr uint32_t AR_COND_ACTION_MASK = 0b1100'0000;

	std::list<CheatDirective> ParseActionReplayDirectives(std::vector<uint32_t> const& directives);
}