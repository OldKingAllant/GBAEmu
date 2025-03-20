#include "../../ppu/PPU.hpp"
#include "../../common/Error.hpp"
#include "../../common/SSE.hpp"

namespace GBA::ppu {
	using namespace common;

	namespace mode5 {
		static constexpr u32 REG_REF_X = 0x28;
		static constexpr u32 REG_REF_Y = 0x2C;
		static constexpr u32 PIXEL_SIZE = 2;
		static constexpr u32 FRAME_0_START = 0x0;
		static constexpr u32 FRAME_1_START = 0xA000;
		static constexpr u32 FRAME_SIZE = 0x9FFF;
		static constexpr u32 BG2_CNT = 0xC;
		static constexpr u32 FRAME_W = 160;
		static constexpr u32 FRAME_H = 128;
	}

	void PPU::Mode5(u16 lcd_y) {
		bool bg2_enable   = (m_ctx_saved.m_control >> 10) & 1;
		bool forced_blank = (m_ctx_saved.m_control >> 7) & 1;
		bool frame_select = (m_ctx_saved.m_control >> 4) & 1;

		bool mosaic = (ReadSavedRegister16(mode5::BG2_CNT / 2) >> 6) & 1;

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
			vram_offset = mode5::FRAME_1_START;

		if (forced_blank) {
			for (unsigned x = 0; x < 240; x++) {
				m_framebuffer[framebuffer_y + x * 3] = 1.0f;
				m_framebuffer[framebuffer_y + x * 3 + 1] = 1.0f;
				m_framebuffer[framebuffer_y + x * 3 + 2] = 1.0f;
			}

			return;
		}

		for (unsigned x = 0; x < 240; x++) {
			int tex_x = x;
			int tex_y = curr_line;

			u16 color_packed = m_palette_ram[BG_PALETTE_START];
			color_packed |= ((u16)m_palette_ram[BG_PALETTE_START + 1] << 8);

			if (tex_x < mode5::FRAME_W && tex_y < mode5::FRAME_H &&
				tex_x >= 0 && tex_y >= 0) {
				u32 vram_pos = vram_offset + 
					   tex_x * mode5::PIXEL_SIZE
					+ (tex_y * 160 * mode5::PIXEL_SIZE);

				color_packed = *reinterpret_cast<u16*>(m_vram + vram_pos);
			}

			u8 r = color_packed & 0x1F;
			u8 g = (color_packed >> 5) & 0x1F;
			u8 b = (color_packed >> 10) & 0x1F;

			m_framebuffer[framebuffer_y + x * 3] = (float)r / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 1] = (float)g / 0x1F;
			m_framebuffer[framebuffer_y + x * 3 + 2] = (float)b / 0x1F;
		}
	}

	void PPU::Mode5_Batch(u16 first_line, u16 last_line) {
		bool bg2_enable = (m_ctx_saved.m_control >> 10) & 1;
		bool forced_blank = (m_ctx_saved.m_control >> 7) & 1;
		bool frame_select = (m_ctx_saved.m_control >> 4) & 1;

		if (!bg2_enable)
			return;

		unsigned framebuffer_pos = first_line * 240 * 3;

		u32 vram_offset = 0;

		if (frame_select)
			vram_offset = mode5::FRAME_1_START;

		if (forced_blank) {
			for (; first_line != last_line; first_line++) {
				std::fill_n(&m_framebuffer[framebuffer_pos], 240 * 3, 1.0f);
				framebuffer_pos += 240 * 3;
			}

			return;
		}

		auto palette_ram16  = std::bit_cast<u16*>(m_palette_ram);
		u16 backdrop_packed = palette_ram16[BG_PALETTE_START];

		struct BackdropPixel {
			float r, g, b;
		};

		BackdropPixel backdrop_pixel{
			.r = (backdrop_packed >> 0 & 0x1F) / 31.f,
			.g = ((backdrop_packed >> 5) & 0x1F) / 31.f,
			.b = ((backdrop_packed >> 10) & 0x1F) / 31.f
		};

		for (; first_line != last_line && first_line < mode5::FRAME_H; first_line++) {
			if constexpr (has_sse42()) {
				unsigned x = 0;
				auto vram_pos = vram_offset + (first_line * 160 * mode5::PIXEL_SIZE);
				u16 packed_colors[8] = {};

				for (; x < mode5::FRAME_W; x += 8) {
					auto abs_vram_pos = vram_pos +
						x * mode5::PIXEL_SIZE;

					auto mm_packed_colors = _mm_loadu_epi16(&m_vram[abs_vram_pos]);
					_mm_storeu_epi16(packed_colors, mm_packed_colors);

					for (auto pixel = 0; pixel < 8; ++pixel) {
						u16 color_packed = packed_colors[pixel];

						u8 r = color_packed & 0x1F;
						u8 g = (color_packed >> 5) & 0x1F;
						u8 b = (color_packed >> 10) & 0x1F;

						m_framebuffer[framebuffer_pos + 0] = (float)r / 0x1F;
						m_framebuffer[framebuffer_pos + 1] = (float)g / 0x1F;
						m_framebuffer[framebuffer_pos + 2] = (float)b / 0x1F;
						framebuffer_pos += 3;
					}
				}

				for (; x < 240; x++) {
					m_framebuffer[framebuffer_pos + 0] = backdrop_pixel.r;
					m_framebuffer[framebuffer_pos + 1] = backdrop_pixel.g;
					m_framebuffer[framebuffer_pos + 2] = backdrop_pixel.b;
					framebuffer_pos += 3;
				}
			}
			else {
				for (unsigned x = 0; x < 240; x++) {
					int tex_x = x;
					int tex_y = first_line;

					u16 color_packed = backdrop_packed;

					if (tex_x < mode5::FRAME_W) {
						u32 vram_pos = vram_offset +
							  tex_x * mode5::PIXEL_SIZE
							+ (tex_y * 160 * mode5::PIXEL_SIZE);

						color_packed = *reinterpret_cast<u16*>(m_vram + vram_pos);
					}

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

		for (; first_line < VISIBLE_LINES; first_line++) {
			for (auto x = 0; x < 240; x++) {
				m_framebuffer[framebuffer_pos + 0] = backdrop_pixel.r;
				m_framebuffer[framebuffer_pos + 1] = backdrop_pixel.g;
				m_framebuffer[framebuffer_pos + 2] = backdrop_pixel.b;
				framebuffer_pos += 3;
			}
		}
	}
}