#pragma once

#include <cstdint>

#if defined(__x86_64__) || defined(_M_AMD64)
#include <immintrin.h>
#include <xmmintrin.h>
#endif

namespace GBA::common {
	static constexpr bool has_sse42() {
#if defined(__x86_64__) || defined(_M_AMD64)
		return true;
#else
		return false;
#endif // AMD64
	}
}
