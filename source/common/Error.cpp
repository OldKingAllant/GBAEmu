#include "../../common/Error.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
	#include <intrin.h>
	#include <Windows.h>
#endif

namespace GBA::error {
	void Assert(bool value, const char* message) {
		if (value)
			return;

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
		MessageBoxA(0, message, "Assertion error", 0);
#else
		std::cerr << message << std::endl;
#endif

		std::abort();
	}

	[[noreturn]] void DebugBreak() {
#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
		__debugbreak();
#else 
#if defined(__x86_64__) || defined(_M_AMD64)
		__asm {
			int 3
		}
#else 
#error "Unsupported architecture"
#endif
#endif // DEBUG
	}

	[[noreturn]] void BailOut(bool bad) {
		if (bad)
			std::abort();

		std::exit(0);
	}
}