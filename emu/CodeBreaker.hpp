#pragma once

#include "Cheats.hpp"

namespace GBA::cheats {
	enum class CB_OpcodeMatch {
		GAME_ID  = 0x0,
		HOOK     = 0x1,
		WRITE_8  = 0x3,
		SLIDE_16 = 0x4,
		IF_EQ    = 0x7,
		WRITE_16 = 0x8,
		ENCRYPT  = 0x9
	};

	std::list<CheatDirective> ParseCodeBreakerDirectives(std::vector<uint32_t> const& directives);
}