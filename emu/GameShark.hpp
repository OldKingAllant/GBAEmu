#pragma once

#include "../common/Defs.hpp"
#include "Cheats.hpp"

#include <list>

namespace GBA::cheats {
	enum class GameSharkCodes {
		GAME_ID,
		HOOK_ROUTINE,
		CHANGE_ENCRYPTION
	};

	enum class GameSharkValues : uint64_t {
		GAME_ID      = 0x0000'0000'001D'C0DE,
		HOOK_ROUTINE = 0xF000'0000'0000'0000
	};

	std::list<CheatDirective> ParseGameSharkDirectives(std::vector<uint32_t> const& directives);
}