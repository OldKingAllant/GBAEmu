#pragma once

#include "../common/Defs.hpp"

#include <vector>
#include <list>
#include <string>

namespace GBA::emulation {
	class Emulator;
}

namespace GBA::cheats {
	enum class CheatType {
		GAMESHARK,
		ACTION_REPLAY
	};

	enum class DirectiveType {
		NONE,
		WRITE_RAM_8,
		WRITE_RAM_16,
		WRITE_RAM_32,
		IF_BLOCK
	};

	namespace directive_detail {
		struct Nop {};

		struct RamWrite8 {
			uint32_t address;
			uint8_t value;
		};

		struct IfBlock {

		};

		union DirectiveCommand {
			Nop nop;
			RamWrite8 ram_write8;
		};
	}

	struct CheatDirective {
		DirectiveType ty = DirectiveType::NONE;
		directive_detail::DirectiveCommand cmd = { .nop = {} };
	};

	struct CheatSet {
		CheatType ty;
		std::list<CheatDirective> directives = {};
	};

	CheatSet ParseCheat(std::vector<std::string> const& lines, CheatType ty);

	bool RunCheatInterpreter(CheatSet& cheat_set, emulation::Emulator* emu);
}