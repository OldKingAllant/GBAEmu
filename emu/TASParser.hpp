#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>

#include "../common/Defs.hpp"
#include "../memory/Keypad.hpp"

namespace GBA::tas {
	using namespace GBA::common;

	static const std::unordered_map<std::string, u16> g_ACTION_BUTTON_MAP = {
		std::pair<std::string, u16>{"w", 0},
		{"a", u16(input::Buttons::BUTTON_A)},
		{"b", u16(input::Buttons::BUTTON_B)},
		{"st", u16(input::Buttons::BUTTON_START)},
		{"sel", u16(input::Buttons::BUTTON_SELECT)},
		{"d", u16(input::Buttons::BUTTON_DOWN)},
		{"u", u16(input::Buttons::BUTTON_UP)},
		{"l", u16(input::Buttons::BUTTON_LEFT)},
		{"r", u16(input::Buttons::BUTTON_RIGHT)},
		{"rt", u16(input::Buttons::BUTTON_R)},
		{"lt", u16(input::Buttons::BUTTON_L)},
		{"ub", u16(input::Buttons::BUTTON_UP) | u16(input::Buttons::BUTTON_B)},
		{"db", u16(input::Buttons::BUTTON_DOWN) | u16(input::Buttons::BUTTON_B)},
		{"rb", u16(input::Buttons::BUTTON_RIGHT) | u16(input::Buttons::BUTTON_B)},
		{"lb", u16(input::Buttons::BUTTON_LEFT) | u16(input::Buttons::BUTTON_B)}
	};

	struct Action {
		u16 buttons;
		u32 num_frames;
	};

	std::optional<std::vector<Action>> _ParseFile(std::filesystem::path const& path);
}