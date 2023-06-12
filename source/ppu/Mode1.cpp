#include "../../ppu/PPU.hpp"

namespace GBA::ppu {
	using namespace common;

	namespace mode1 {
		std::array<Pixel, 240> bg_1_data{};
		std::array<Pixel, 240> bg_2_data{};
		std::array<Pixel, 240> bg_3_data{};
		std::array<Pixel, 240> bg_4_data{};
		std::array<Pixel, 240> obj_data{};
	}

	void PPU::Mode1() {
		u16 curr_line = m_ctx.m_vcount;

		unsigned framebuffer_y = curr_line * 240 * 3;

		bool bg_1 = (m_ctx.m_control >> 8) & 1;
		bool bg_2 = (m_ctx.m_control >> 9) & 1;
		bool bg_3 = (m_ctx.m_control >> 10) & 1;
		bool obj_enable = (m_ctx.m_control >> 12) & 1;

		mode1::bg_1_data = {};
		mode1::bg_2_data = {};
		mode1::bg_3_data = {};
		mode1::bg_4_data = {};
		mode1::obj_data = {};

		if (bg_1)
			mode1::bg_1_data = ProcessNormalBackground(0, curr_line);

		if (bg_2)
			mode1::bg_2_data = ProcessNormalBackground(1, curr_line);

		if (bg_3)
			mode1::bg_3_data = ProcessAffineBackground(2, curr_line);

		if (obj_enable)
			mode1::obj_data = DrawSprites(curr_line);


		std::array<Pixel, 240> bg_data = MergeBackrounds(
			mode1::bg_1_data, mode1::bg_2_data, mode1::bg_3_data, mode1::bg_4_data,
			mode1::obj_data
		);

		u16 backdrop = *reinterpret_cast<u16*>(m_palette_ram);

		u16 color_packed = 0;

		for (int x = 0; x < 240; x++) {
			u16 color_packed = backdrop;

			if (bg_data[x].palette_id)
				color_packed = bg_data[x].color;

			u8 r = color_packed & 0x1F;
			u8 g = (color_packed >> 5) & 0x1F;
			u8 b = (color_packed >> 10) & 0x1F;

			m_framebuffer[framebuffer_y + x * 3] = (float)r / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 1] = (float)g / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 2] = (float)b / 0x1F;
		}
	}
}