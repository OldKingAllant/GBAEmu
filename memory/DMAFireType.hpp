#pragma once

#include "../common/Defs.hpp"

namespace GBA::memory {
	enum class DMAFireType : common::u8 {
		HBLANK,
		VBLANK,
		FIFO_A,
		FIFO_B,
		CAPTURE
	};
}
