#pragma once

namespace GBA::error {
	void Assert(bool value, const char* message);
	[[noreturn]] void DebugBreak();
	[[noreturn]] void BailOut(bool bad);

	[[noreturn]] inline void Unreachable() {
#if defined(_MSC_VER) || defined(MSVC)
		__assume(false);
#elif defined(__GNUC__)
		__builtin_unreachable();
#endif
	}
}