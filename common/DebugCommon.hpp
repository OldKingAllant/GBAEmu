#pragma once

#include <string>
#include "./Defs.hpp"

namespace GBA::debugger {
	using namespace common;

	static std::string_view BoolToString(bool value) {
		return value ? "true" : "false";
	}

	static std::string IntToString(u32 value) {
		return std::to_string(value);
	}
}