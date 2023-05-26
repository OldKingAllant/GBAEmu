#include "../../ppu/PPU.hpp"

namespace GBA::ppu {
	using namespace common;

	namespace mode0 {
		//
	}

	void PPU::Mode0() {
		u16 curr_line = m_ctx.m_vcount;

		unsigned framebuffer_y = curr_line * 240 * 3;

		bool bg_1 = (m_ctx.m_control >> 8) & 1;
		bool bg_2 = (m_ctx.m_control >> 9) & 1;
		bool bg_3 = (m_ctx.m_control >> 10) & 1;
		bool bg_4 = (m_ctx.m_control >> 11) & 1;

		std::array<Pixel, 240> pixels{};

		if (bg_1)
			pixels = ProcessNormalBackground(0, curr_line);

		if (bg_2)
			pixels = ProcessNormalBackground(1, curr_line);

		if (bg_3)
			pixels = ProcessNormalBackground(2, curr_line);

		if (bg_4)
			pixels = ProcessNormalBackground(3, curr_line);

		for (int x = 0; x < 240; x++) {
			u16 backdrop = *reinterpret_cast<u16*>(m_palette_ram);

			u16 color_packed = backdrop;

			if (pixels[x].palette_id)
				color_packed = pixels[x].color;

			u8 r = color_packed & 0x1F;
			u8 g = (color_packed >> 5) & 0x1F;
			u8 b = (color_packed >> 10) & 0x1F;

			m_framebuffer[framebuffer_y + x * 3] = (float)r / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 1] = (float)g / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 2] = (float)b / 0x1F;
		}
	}
}