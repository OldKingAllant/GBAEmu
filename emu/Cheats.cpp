#include "Cheats.hpp"

#include "GameShark.hpp"
#include "ActionReplay.hpp"

#include "Emulator.hpp"

#include "../common/Error.hpp"

#include <fmt/format.h>

namespace GBA::cheats {
	static std::vector<uint32_t> ParseHex(std::string const& line) {
		std::vector<uint32_t> directives{};
		uint32_t curr_digit = 0;
		uint32_t curr_val = 0;

		auto to_digit = [](char ch) {
			if (ch >= 'a' && ch <= 'f')
				return ch - 'a' + 0xA;
			return ch - '0';
		};

		for (char ch : line) {
			ch = tolower(ch);

			if (!isxdigit(ch))
				return {};

			uint32_t digit = to_digit(ch);
			curr_val |= (digit << (28 - curr_digit * 4));

			if (++curr_digit == 8) {
				directives.push_back(curr_val);
				curr_digit = 0;
				curr_val = 0;
			}
		}

		return directives;
	}

	CheatSet ParseCheat(std::vector<std::string> const& lines, CheatType ty) {
		std::vector<uint32_t> directives{};

		for (auto const& line : lines) {
			std::string new_line = line;
			new_line.erase(
				std::remove(
					new_line.begin(),
					new_line.end(),
					' '),
				new_line.end()
			);

			auto parsed_directives = ParseHex(new_line);
			directives.insert(directives.end(),
				parsed_directives.cbegin(),
				parsed_directives.cend());
		}

		CheatSet set{};

		set.ty = ty;

		switch (ty)
		{
		case CheatType::GAMESHARK:
			set.directives = ParseGameSharkDirectives(directives);
			break;
		case CheatType::ACTION_REPLAY:
			set.directives = ParseActionReplayDirectives(directives);
			break;
		default:
			break;
		}

		return set;
	}

	bool RunCheatInterpreter(CheatSet& cheat_set, emulation::Emulator* emu) {
		auto directive_iter = cheat_set.directives.begin();

		while(directive_iter != cheat_set.directives.end()) {
			switch (directive_iter->ty)
			{
			case DirectiveType::NONE:
				break;
			case DirectiveType::WRITE_RAM_8: {
				auto& cmd = directive_iter->cmd.ram_write8;
				auto address = cmd.address;
				auto value = cmd.value;
				emu->GetContext().bus.Write(address, value);
			}
				break;
			default:
				fmt::println("[CHEATS] Encountered unknown instruction");
				error::DebugBreak();
				break;
			}

			directive_iter++;
		}

		return true;
	}
}