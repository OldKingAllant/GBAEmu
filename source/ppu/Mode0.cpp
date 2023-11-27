#include "../../ppu/PPU.hpp"

namespace GBA::ppu {
	using namespace common;

	namespace mode0 {
		std::array<Pixel, 240> bg_0;
		std::array<Pixel, 240> bg_1;
		std::array<Pixel, 240> bg_2;
		std::array<Pixel, 240> bg_3;
	}

	void PPU::Mode0() {
		u16 curr_line = m_ctx.m_vcount;

		unsigned framebuffer_y = curr_line * 240 * 3;

		bool bg_1 = (m_ctx.m_control >> 8) & 1;
		bool bg_2 = (m_ctx.m_control >> 9) & 1;
		bool bg_3 = (m_ctx.m_control >> 10) & 1;
		bool bg_4 = (m_ctx.m_control >> 11) & 1;
		bool obj_enable = (m_ctx.m_control >> 12) & 1;

		m_line_data[4] = {};

		if (bg_1)
			ProcessNormalBackground(0, curr_line);

		if (bg_2)
			ProcessNormalBackground(1, curr_line);

		if (bg_3)
			ProcessNormalBackground(2, curr_line);

		if (bg_4)
			ProcessNormalBackground(3, curr_line);

		if (obj_enable)
			DrawSprites(curr_line);

		std::array<Pixel, 240> pixels = MergeBackrounds();

		for (int x = 0; x < 240; x++) {
			u16 color_packed = pixels[x].color;

			u8 r = color_packed & 0x1F;
			u8 g = (color_packed >> 5) & 0x1F;
			u8 b = (color_packed >> 10) & 0x1F;

			m_framebuffer[framebuffer_y + x * 3] = (float)r / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 1] = (float)g / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 2] = (float)b / 0x1F;
		}
	}
}