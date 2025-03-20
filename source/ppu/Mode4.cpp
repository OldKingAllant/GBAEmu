#include "../../ppu/PPU.hpp"
#include "../../common/Error.hpp"
#include "../../common/SSE.hpp"

namespace GBA::ppu {
	using namespace common;

	namespace mode4 {
		static constexpr u32 REG_REF_X = 0x28;
		static constexpr u32 REG_REF_Y = 0x2C;
		static constexpr u32 PIXEL_SIZE = 1;
		static constexpr u32 FRAME_0_START = 0x0;
		static constexpr u32 FRAME_1_START = 0xA000;
		static constexpr u32 FRAME_SIZE = 0x9FFF;
		static constexpr u32 BG2_CNT = 0xC;
		static constexpr u32 FRAME_W = 240;
		static constexpr u32 FRAME_H = 160;
	}

	void PPU::Mode4(u16 lcd_y) {
		bool bg2_enable   = (m_ctx_saved.m_control >> 10) & 1;
		bool forced_blank = (m_ctx_saved.m_control >> 7) & 1;
		bool frame_select = (m_ctx_saved.m_control >> 4) & 1;

		bool mosaic = (ReadSavedRegister16(mode4::BG2_CNT / 2) >> 6) & 1;

		if (!bg2_enable)
			return;

		unsigned curr_line = lcd_y;

		if (curr_line >= 160) {
			error::DebugBreak();
		}

		if (mosaic)
			error::DebugBreak();
			
		unsigned framebuffer_y = curr_line * 240 * 3;

		u32 vram_offset = 0;

		if (frame_select)
			vram_offset = mode4::FRAME_1_START;

		if (forced_blank) {
			for (unsigned x = 0; x < 240; x++) {
				m_framebuffer[framebuffer_y + x * 3] = 1.0f;
				m_framebuffer[framebuffer_y + x * 3 + 1] = 1.0f;
				m_framebuffer[framebuffer_y + x * 3 + 2] = 1.0f;
			}

			return;
		}

		std::array<Pixel, 240> bg2{};

		for (unsigned x = 0; x < 240; x++) {
			int tex_x = x;
			int tex_y = curr_line;

			u16 color_packed = m_palette_ram[BG_PALETTE_START];
			color_packed |= ((u16)m_palette_ram[BG_PALETTE_START + 1] << 8);

			u8 palette_index = 0;

			if (tex_x < mode4::FRAME_W && tex_y < mode4::FRAME_H &&
				tex_x >= 0 && tex_y >= 0) {
				u32 vram_pos = tex_x * mode4::PIXEL_SIZE
					+ (tex_y * 240 * mode4::PIXEL_SIZE);

				palette_index = m_vram[vram_offset + vram_pos];

				color_packed = m_palette_ram[BG_PALETTE_START + palette_index * 2];
				color_packed |= ((u16)m_palette_ram[BG_PALETTE_START + palette_index * 2 + 1] << 8);
			}

			bg2[x].palette_id = palette_index;
			bg2[x].color = color_packed;
		}

		bool obj_enable = (m_ctx_saved.m_control >> 12) & 1;

		m_line_data[4] = {};

		if (obj_enable) {
			DrawSprites(curr_line);
			bg2 = MergeBitmap(lcd_y, bg2, m_line_data[4]);
		}

		for (unsigned x = 0; x < 240; x++) {
			u16 color_packed = bg2[x].color;

			u8 r = color_packed & 0x1F;
			u8 g = (color_packed >> 5) & 0x1F;
			u8 b = (color_packed >> 10) & 0x1F;

			m_framebuffer[framebuffer_y + x * 3] = (float)r / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 1] = (float)g / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 2] = (float)b / 0x1F;
		}
	}

	void PPU::Mode4_Batch(u16 first_line, u16 last_line) {
		bool bg2_enable   = (m_ctx_saved.m_control >> 10) & 1;
		bool forced_blank = (m_ctx_saved.m_control >> 7) & 1;
		bool frame_select = (m_ctx_saved.m_control >> 4) & 1;

		if (!bg2_enable) {
			return;
		}
		
		unsigned framebuffer_pos = first_line * (240 * 3);

		u32 vram_offset = 0;

		if (frame_select) {
			vram_offset = mode4::FRAME_1_START;
		}

		if (forced_blank) {
			for (; first_line != last_line; first_line++) {
				std::fill_n(&m_framebuffer[framebuffer_pos], 240 * 3, 1.0f);
				framebuffer_pos += 240 * 3;
			}
			
			return;
		}

		auto palette_ram16 = std::bit_cast<u16*>(m_palette_ram);

		for (; first_line != last_line; first_line++) {
			if constexpr (has_sse42()) {
				auto vram_pos = (first_line * 240 * mode4::PIXEL_SIZE);

				for (unsigned x = 0; x < 240; x += 16) {
					auto abs_vram_pos = vram_offset + vram_pos + x;

					//pal_indices = indices in palette for 16 consecutive pixels
					auto pal_indices = _mm_loadu_epi8(&m_vram[abs_vram_pos]);
					u8 indices[16] = {};
					_mm_storeu_epi8(indices, pal_indices);

					for (auto pixel = 0; pixel < 16; ++pixel) {
						u16 color_packed = palette_ram16[indices[pixel]];

						u8 r = color_packed & 0x1F;
						u8 g = (color_packed >> 5) & 0x1F;
						u8 b = (color_packed >> 10) & 0x1F;

						m_framebuffer[framebuffer_pos + 0] = (float)r / 0x1F;
						m_framebuffer[framebuffer_pos + 1] = (float)g / 0x1F;
						m_framebuffer[framebuffer_pos + 2] = (float)b / 0x1F;
						framebuffer_pos += 3;
					}
				}
			}
			else {
				for (unsigned x = 0; x < 240; x++) {
					u32 tex_x{ x };
					u32 tex_y{ first_line };

					u32 vram_pos = tex_x * mode4::PIXEL_SIZE
						+ (tex_y * 240 * mode4::PIXEL_SIZE);

					u8 palette_index = m_vram[vram_offset + vram_pos];

					u16 color_packed = m_palette_ram[BG_PALETTE_START + palette_index * 2];
					color_packed |= ((u16)m_palette_ram[BG_PALETTE_START + palette_index * 2 + 1] << 8);

					u8 r = color_packed & 0x1F;
					u8 g = (color_packed >> 5) & 0x1F;
					u8 b = (color_packed >> 10) & 0x1F;

					m_framebuffer[framebuffer_pos + 0] = (float)r / 0x1F;
					m_framebuffer[framebuffer_pos + 1] = (float)g / 0x1F;
					m_framebuffer[framebuffer_pos + 2] = (float)b / 0x1F;
					framebuffer_pos += 3;
				}
			}
			
		}
	}
}