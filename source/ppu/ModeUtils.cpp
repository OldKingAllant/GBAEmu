#include "../../ppu/PPU.hpp"

namespace GBA::ppu {
	using namespace common;

	void PPU::CalculateMosaicBG(u32& x, u32& y) {
		u32 mosaic_reg = ReadRegister32(0x4C / 4);

		u8 h_size = (mosaic_reg & 0xF) + 1;
		u8 v_size = ((mosaic_reg >> 4) & 0xF) + 1;

		if (!h_size || !v_size)
			return;

		x -= (x % h_size);
		y -= (y % v_size);
	}
}