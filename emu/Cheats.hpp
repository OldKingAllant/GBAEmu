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
		IF_BLOCK,
		INDIRECT_WRITE_8,
		INDIRECT_WRITE_16,
		INDIRECT_WRITE_32
	};

	enum class Condition {
		EQ,
		NE,
		LT,
		GT,
		LTU,
		GTU,
		AND
	};

	enum class ConditionOperand {
		SIZE_8,
		SIZE_16,
		SIZE_32,
		ALWAYS_FALSE
	};

	namespace directive_detail {
		struct Nop {};

		struct RamWrite8 {
			uint32_t address;
			uint8_t value;
		};

		struct IfBlock {
			Condition cond;
			ConditionOperand operand_size;
			bool has_else;
			uint32_t else_location;
			uint32_t else_size;
			uint32_t then_size;
			uint32_t address;
			uint32_t operand;
		};

		struct IndirectWrite16 {
			uint32_t address;
			uint32_t offset;
			uint16_t value;
		};

		union DirectiveCommand {
			Nop			   nop;
			RamWrite8	   ram_write8;
			IfBlock		   if_block;
			IndirectWrite16 indirect16;
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