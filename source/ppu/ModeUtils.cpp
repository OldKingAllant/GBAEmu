#include "../../ppu/PPU.hpp"

#include "../../common/Error.hpp"

namespace GBA::ppu {
	using namespace common;

	namespace detail {
		constexpr u16 bg_control_reg[] = {
			0x8, 0xA, 0xC, 0xE
		};

		constexpr u16 bg_scroll_reg[][2] = {
			{ 0x10, 0x12 },
			{ 0x14, 0x16 },
			{ 0x18, 0x1A },
			{ 0x1C, 0x1E }
		};

		constexpr u16 bg_tile_size[] = {
			0x20, 0x40
		};

		constexpr u16 bg_size[][2] = {
			{ 256, 256 },
			{ 512, 256 },
			{ 256, 512 },
			{ 512, 512 }
		};

		constexpr u16 bg_tilemap_size[][2] = {
			{ 32, 32 },
			{ 64, 32 },
			{ 32, 64 },
			{ 64, 64 }
		};

		constexpr u16 TILE_REGION_X_SIZE = 32;
		constexpr u16 TILE_REGION_Y_SIZE = 32;

		constexpr u16 TILE_X_SIZE = 8;
		constexpr u16 TILE_Y_SIZE = 8;
	}

	void PPU::CalculateMosaicBG(i32& x, i32& y) {
		u32 mosaic_reg = ReadRegister32(0x4C / 4);

		u8 h_size = (mosaic_reg & 0xF) + 1;
		u8 v_size = ((mosaic_reg >> 4) & 0xF) + 1;

		if (!h_size || !v_size)
			return;

		x -= (x % h_size);
		y -= (y % v_size);
	}

	std::array<PPU::Pixel, 240> PPU::ProcessNormalBackground(int bg_id, int lcd_y) {
		std::array<PPU::Pixel, 240> backround_data{};

		u16 bg_control = ReadRegister16(detail::bg_control_reg[bg_id] / 2);

		u32 char_base_block = (bg_control >> 2) & 0x3;
		char_base_block *= 16 * 1024;

		bool mosaic = (bg_control >> 6) & 1;
		bool pal_mode = (bg_control >> 7) & 1;

		u32 map_base_block = (bg_control >> 8) & 0x1F;
		map_base_block *= 2 * 1024;

		u8 screen_sz_type = (bg_control >> 14) & 0x3;

		u32 bg_size_x = detail::bg_size[screen_sz_type][0];
		u32 bg_size_y = detail::bg_size[screen_sz_type][1];

		u32 scroll_x = ReadRegister16(detail::bg_scroll_reg[bg_id][0] / 2) & 511;
		u32 scroll_y = ReadRegister16(detail::bg_scroll_reg[bg_id][1] / 2) & 511;

		u32 bg_x = 0;
		u32 bg_y = scroll_y + (u32)lcd_y;
		bg_y %= bg_size_y;
		u32 y_offset_inside_tile = (scroll_y + (u32)lcd_y) % detail::TILE_Y_SIZE;

		u32 tile_y = bg_y / detail::TILE_Y_SIZE;
		u32 x_offset_inside_tile = 0;
		u32 tile_x = 0;
		u32 tile_region_y = tile_y / detail::TILE_REGION_Y_SIZE;
		u32 tile_region_x = 0;

		u32 total_tile_regions_x = detail::bg_tilemap_size[screen_sz_type][0] / 32;

		u32 tile_row_sz = 4;
		u32 tile_data_size = 0x20;

		if (pal_mode) {
			tile_row_sz = 8;
			tile_data_size = 0x40;
		}
			

		for (int x = 0; x < 240; x++) {
			/*Find tilemap entry*/
			bg_x = scroll_x + x;
			bg_x %= bg_size_x;
			x_offset_inside_tile = bg_x % detail::TILE_X_SIZE;
			tile_x = bg_x / detail::TILE_X_SIZE;

			tile_region_x = tile_x / detail::TILE_REGION_X_SIZE;

			u32 tile_region_id = total_tile_regions_x * tile_region_y
				+ tile_region_x;

			tile_x %= detail::TILE_REGION_X_SIZE;
			tile_y %= detail::TILE_REGION_Y_SIZE;

			u32 vram_offset = map_base_block + (tile_region_id * 2048) + 
				(tile_y * 64) + tile_x * 2;

			u16 tile_entry = *reinterpret_cast<u16*>(m_vram + vram_offset);

			u32 tile_id = tile_entry & 1023;
			bool hflip = (tile_entry >> 10) & 1;
			bool vflip = (tile_entry >> 11) & 1;
			u32 pal_id = (tile_entry >> 12) & 0xF;

			u32 real_y_offset_in_tile = y_offset_inside_tile;

			if (hflip)
				x_offset_inside_tile = 7 - x_offset_inside_tile;
			
			if (vflip)
				real_y_offset_in_tile = 7 - y_offset_inside_tile;

			vram_offset = char_base_block + tile_id * tile_data_size
				+ (tile_row_sz * real_y_offset_in_tile);

			if (pal_mode)
				vram_offset += x_offset_inside_tile;
			else
				vram_offset += x_offset_inside_tile / 2;

			u16 color_id = m_vram[vram_offset];

			u16 color = 0;

			if (pal_mode) {
				color = *reinterpret_cast<u16*>(m_palette_ram + color_id * 2);
				backround_data[x].color = color;
				backround_data[x].palette_id = color_id;
			}
			else {
				pal_id *= 32;

				u16 color_1 = (color_id & 0xF) * 2;
				u16 color_2 = ((color_id >> 4) & 0xF) * 2;

				backround_data[x].color = *reinterpret_cast<u16*>(m_palette_ram + pal_id + color_1);
				backround_data[x + 1].color = *reinterpret_cast<u16*>(m_palette_ram + pal_id + color_2);

				backround_data[x].palette_id = color_1;
				backround_data[x + 1].palette_id = color_2;

				x++;
			}
		}

		return backround_data;
	}
}