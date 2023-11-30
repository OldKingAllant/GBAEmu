#include "../../ppu/PPU.hpp"

#include "../../common/Error.hpp"

#include <algorithm>

namespace GBA::ppu {
	using namespace common;

	namespace detail {
		constexpr u16 bg_control_reg[] = {
			0x8, 0xA, 0xC, 0xE
		};

		constexpr u16 bg_rot_scale_reg[][2] = {
			{ }, 
			{ }, 
			{ 0x28, 0x2C },
			{ 0x38, 0x3C }
		};

		constexpr u16 bg_aff_param_reg[][4] = {
			{ },
			{ },
			{ 0x20, 0x22, 0x24, 0x26 },
			{ 0x30, 0x32, 0x34, 0x36 }
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

		constexpr u16 bg_aff_size[][2] = {
			{ 128, 128 }, 
			{ 256, 256 },
			{ 512, 512 },
			{ 1024, 1024 }
		};

		constexpr u16 bg_tilemap_size[][2] = {
			{ 32, 32 },
			{ 64, 32 },
			{ 32, 64 },
			{ 64, 64 }
		};

		constexpr u16 bg_aff_tilemap_size[][2] = {
			{ 16, 16 },
			{ 32, 32 },
			{ 64, 64 },
			{ 128, 128 }
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

	std::array<std::array<Pixel, 240>, 5> backgrounds{};

	void PPU::ProcessNormalBackground(int bg_id, int lcd_y) {
		u16 bg_control = ReadRegister16(detail::bg_control_reg[bg_id] / 2);

		u8 bg_prio = bg_control & 3;

		u32 char_base_block = (bg_control >> 2) & 0x3;
		char_base_block *= 16 * 1024;

		bool mosaic = (bg_control >> 6) & 1;
		bool pal_mode = (bg_control >> 7) & 1;

		u32 mos_h_size = 1;
		u32 mos_v_size = 1;

		if (mosaic) {
			u32 mos_cnt = ReadRegister32(0x4C / 4);

			mos_h_size = (mos_cnt & 0xF) + 1;
			mos_v_size = ((mos_cnt >> 4) & 0xF) + 1;
		}

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
		bg_y -= (bg_y % mos_v_size);

		if ((i32)bg_y < 0)
			bg_y = 0;

		u32 y_offset_inside_tile = bg_y % detail::TILE_Y_SIZE;

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

		u16* u16_vram_ptr = std::bit_cast<u16*>(m_vram);
		u16* u16_palette_ptr = std::bit_cast<u16*>(m_palette_ram);
			

		for (int x = 0; x < 240; /*x++*/) {
			/*Find tilemap entry*/
			bg_x = scroll_x + x;
			bg_x %= bg_size_x;
			bg_x -= (bg_x % mos_h_size);

			x_offset_inside_tile = bg_x % detail::TILE_X_SIZE;
			tile_x = bg_x / detail::TILE_X_SIZE;

			tile_region_x = tile_x / detail::TILE_REGION_X_SIZE;

			u32 tile_region_id = total_tile_regions_x * tile_region_y
				+ tile_region_x;

			tile_x %= detail::TILE_REGION_X_SIZE;
			tile_y %= detail::TILE_REGION_Y_SIZE;

			u32 vram_offset = map_base_block + (tile_region_id * 2048) + 
				(tile_y * 64) + tile_x * 2;

			u16 tile_entry = u16_vram_ptr[vram_offset / 2] /**reinterpret_cast<u16*>(m_vram + vram_offset)*/;

			u32 tile_id = tile_entry & 1023;
			bool hflip = (tile_entry >> 10) & 1;
			bool vflip = (tile_entry >> 11) & 1;
			u32 pal_id = (tile_entry >> 12) & 0xF;

			u32 real_y_offset_in_tile = y_offset_inside_tile;

			if (vflip)
				real_y_offset_in_tile = 7 - y_offset_inside_tile;

			vram_offset = char_base_block + tile_id * tile_data_size
				+ (tile_row_sz * real_y_offset_in_tile);

			int offset_inc = 1;

			if (hflip) {
				x_offset_inside_tile = 7 - x_offset_inside_tile;
				offset_inc = -1;
			}

			int x_offset_inside_tile_int = x_offset_inside_tile;

			pal_id *= 16;

			while (x_offset_inside_tile_int >= 0 && x_offset_inside_tile_int < 8
				&& x < 240) {
				u32 end_vram_offset = vram_offset;

				u16 color = 0;

				if (pal_mode) {
					end_vram_offset += x_offset_inside_tile_int;
					u16 color_id = m_vram[end_vram_offset];
					color = u16_palette_ptr[color_id];
					backgrounds[bg_id][x].color = color;
					backgrounds[bg_id][x].palette_id = color_id;
				}
				else {
					end_vram_offset += x_offset_inside_tile_int / 2;

					u16 color_id = m_vram[end_vram_offset];

					u16 color = 0;

					if (x_offset_inside_tile_int % 2)
						color = ((color_id >> 4) & 0xF);
					else
						color = (color_id & 0xF);

					backgrounds[bg_id][x].color = u16_palette_ptr[pal_id + color];
					backgrounds[bg_id][x].palette_id = color;
				}

				u32 mos_end = x + mos_h_size - 1;

				for (u32 pos = x; pos < mos_end && pos < 239; pos++, x++) {
					backgrounds[bg_id][pos + 1] = backgrounds[bg_id][pos];
					x_offset_inside_tile_int += offset_inc;
				}

				x++;
				
				x_offset_inside_tile_int += offset_inc;
			}
		}
	}

	void PPU::ProcessAffineBackground(int bg_id, int lcd_y) {
		u16 bg_control = ReadRegister16(detail::bg_control_reg[bg_id] / 2);

		u8 bg_prio = bg_control & 3;

		u32 ch_base_block = (bg_control >> 2) & 3;
		ch_base_block *= 16 * 1024;

		bool mosaic = (bg_control >> 6) & 1;
		bool pal_mode = (bg_control >> 7) & 1;

		u32 map_base_block = (bg_control >> 8) & 0x1F;
		map_base_block *= 2 * 1024;

		u8 screen_sz_type = (bg_control >> 14) & 0x3;

		u32 map_x_size = detail::bg_aff_size[screen_sz_type][0];
		u32 map_y_size = detail::bg_aff_size[screen_sz_type][1];

		bool area_overflow = (bg_control >> 13) & 1;

		i32 ref_x = (i32)m_internal_reference_x[bg_id - 2];
		i32 ref_y = (i32)m_internal_reference_y[bg_id - 2];

		ref_x <<= 4;
		ref_x >>= 4;

		ref_y <<= 4;
		ref_y >>= 4;

		i16 dx = (i16)ReadRegister16(detail::bg_aff_param_reg[bg_id][0] / 2);
		i16 dmx = (i16)ReadRegister16(detail::bg_aff_param_reg[bg_id][1] / 2);
		i16 dy = (i16)ReadRegister16(detail::bg_aff_param_reg[bg_id][2] / 2);
		i16 dmy = (i16)ReadRegister16(detail::bg_aff_param_reg[bg_id][3] / 2);


		//Y position changes with x incresing,
		//we cannot precalculate tile y index

		i32 curr_y = ref_y;
		i32 curr_x = ref_x;

		i32 tex_x = 0;
		i32 tex_y = 0;

		u32 tiles_per_line = detail::bg_aff_tilemap_size[screen_sz_type][0];

		u32 tile_row_sz = 8;
		u32 tile_data_size = 0x40;

		for (int x = 0; x < 240; x++) {
			tex_x = curr_x >> 8;
			tex_y = curr_y >> 8;

			if (tex_x < 0 || tex_y < 0) {
				if (area_overflow) {
					tex_x = map_x_size + tex_x;
					tex_y = map_y_size + tex_y;
				}
				else {
					curr_x += dx;
					curr_y += dy;
					backgrounds[bg_id][x].palette_id = 0;
					continue;
				}
			}
				

			if (((u32)tex_x > map_x_size || (u32)tex_y > map_y_size)
				&& !area_overflow) {
				curr_x += dx;
				curr_y += dy;
				backgrounds[bg_id][x].palette_id = 0;
				continue;
			}

			tex_x %= map_x_size;
			tex_y %= map_y_size;

			u32 y_offset_inside_tile = tex_y % detail::TILE_Y_SIZE;

			u32 tile_y = tex_y / detail::TILE_Y_SIZE;
			u32 x_offset_inside_tile = 0;
			u32 tile_x = 0;

			x_offset_inside_tile = tex_x % detail::TILE_X_SIZE;
			tile_x = tex_x / detail::TILE_X_SIZE;

			u32 vram_offset = map_base_block +
				(tile_y * tiles_per_line) + tile_x;

			u8 tile_number = m_vram[vram_offset];

			vram_offset = ch_base_block + tile_number * tile_data_size
				+ (tile_row_sz * y_offset_inside_tile);

			vram_offset += x_offset_inside_tile;

			//No palette mode can be selected
			//only 256/1 is allowed

			u8 color_id = m_vram[vram_offset];

			u16 color = *reinterpret_cast<u16*>(m_palette_ram + color_id * 2);

			backgrounds[bg_id][x].color = color;
			backgrounds[bg_id][x].palette_id = color_id;

			curr_x += dx;
			curr_y += dy;
		}
		
		m_internal_reference_x[bg_id - 2] = (i32)m_internal_reference_x[bg_id - 2] + dmx;
		m_internal_reference_y[bg_id - 2] = (i32)m_internal_reference_y[bg_id - 2] + dmy;
	}

	std::array<Pixel, 240> PPU::MergeBackrounds() {
		std::array<Pixel, 240> merged{};

		backgrounds[4] = m_line_data[4];

		u16 bg1_cnt = ReadRegister16(0x8 / 2);
		u16 bg2_cnt = ReadRegister16(0xA / 2);
		u16 bg3_cnt = ReadRegister16(0xC / 2);
		u16 bg4_cnt = ReadRegister16(0xE / 2);

		u8 mode = m_ctx.m_control & 0x7;

		//Priority goes from 0 to 3 with 0 highest
		//Between same priority, lower bg id wins

		//Extract bg with highest priority

		//////////////////

		bool layer_enabled_global[5] = {
			((m_ctx.m_control >> 8) & 1) && mode < 2,
			((m_ctx.m_control >> 9) & 1) && mode < 2,
			(m_ctx.m_control >> 10) & 1,
			((m_ctx.m_control >> 11) & 1) && (mode == 0 || mode == 2),
			(m_ctx.m_control >> 12) & 1
		};

		bool win0_en = (m_ctx.m_control >> 13) & 1;
		bool win1_en = (m_ctx.m_control >> 14) & 1;
		bool winobj_en = (m_ctx.m_control >> 15) & 1;

		bool win_enabled = win0_en || win1_en || winobj_en;

		u16 winin_cnt = ReadRegister16(0x48 / 2);
		u16 winout_cnt = ReadRegister16(0x4A / 2);

		bool win0_obj_enable = (winin_cnt >> 4) & 1;
		bool win1_obj_enable = (winin_cnt >> 12) & 1;
		bool winout_obj_enable = (winout_cnt >> 4) & 1;
		bool winobj_obj_enable = (winout_cnt >> 12) & 1;

		u16 win0_h = ReadRegister16(0x40 / 2);
		u16 win1_h = ReadRegister16(0x42 / 2);

		u16 win0_v = ReadRegister16(0x44 / 2);
		u16 win1_v = ReadRegister16(0x46 / 2);

		u16 win0_left = (win0_h >> 8) & 0xFF;
		u16 win0_right = (win0_h & 0xFF) - 1;

		u16 win0_top = (win0_v >> 8) & 0xFF;
		u16 win0_bottom = (win0_v & 0xFF) - 1;

		u16 win1_left = (win1_h >> 8) & 0xFF;
		u16 win1_right = (win1_h & 0xFF) - 1;

		u16 win1_top = (win1_v >> 8) & 0xFF;
		u16 win1_bottom = (win1_v & 0xFF) - 1;

		if (win0_right == 0xFFFF)
			win0_right = 0;

		if (win0_bottom == 0xFFFF)
			win0_bottom = 0;

		if (win1_right == 0xFFFF)
			win1_right = 0;

		if (win1_bottom == 0xFFFF)
			win1_bottom = 0;

		struct WindowInfo {
			bool enabled;
			u16 left;
			u16 right;
			u16 top;
			u16 bottom;
			int layer_enable[5];
			int enable_special_effects;
		};

		WindowInfo windows[5] = {
			WindowInfo { win0_en, win0_left, win0_right, win0_top, win0_bottom, {
				winin_cnt & 1, (winin_cnt >> 1) & 1,
				(winin_cnt >> 2) & 1, (winin_cnt >> 3) & 1,
				win0_obj_enable
			}, (winin_cnt >> 5) & 1 },
			WindowInfo { win1_en, win1_left, win1_right, win1_top, win1_bottom, {
				(winin_cnt >> 8) & 1, (winin_cnt >> 9) & 1,
				(winin_cnt >> 10) & 1, (winin_cnt >> 11) & 1,
				win1_obj_enable
			}, (winin_cnt >> 13) & 1 },
			WindowInfo { win_enabled, 0, 0, 0, 0, {
				winout_cnt & 1, (winout_cnt >> 1) & 1,
				(winout_cnt >> 2) & 1, (winout_cnt >> 3) & 1,
				winout_obj_enable
			}, (winout_cnt >> 5) & 1 },
			WindowInfo { true, 0, 0, 0, 0, {
			}, true },
			WindowInfo {
				winobj_en, 0, 0, 0, 0, {
					(winout_cnt >> 8) & 1, (winout_cnt >> 9) & 1,
					(winout_cnt >> 10) & 1, (winout_cnt >> 1) & 1,
					winobj_obj_enable
				}, (winout_cnt >> 13) & 1
			}
		};

		u16 curr_line = m_ctx.m_vcount;

		bool window0_line = curr_line >= windows[0].top && curr_line < windows[0].bottom && windows[0].enabled;
		bool window1_line = curr_line >= windows[1].top && curr_line < windows[1].bottom && windows[1].enabled;

		bool curr_line_has_window = window0_line || window1_line;

		auto get_current_window_id = [&](u16 x_pos) {
			if (!curr_line_has_window) {
				if (windows[4].enabled && m_obj_window_pixels[x_pos])
					return 4;
				if (windows[2].enabled)
					return 2;
				return 3;
			}

			if (x_pos >= windows[0].left && x_pos < windows[0].right && window0_line)
				return 0;
			else if (x_pos >= windows[1].left && x_pos < windows[1].right && window1_line)
				return 1;
			else if (windows[4].enabled && m_obj_window_pixels[x_pos])
				return 4;
			return 2;
		};

		struct Priority {
			u16 priority;
			u8 layer;
		};

		Priority priorities[4] = {
			Priority { static_cast<u16>(bg1_cnt & 3), 0 },
			Priority { static_cast<u16>(bg2_cnt & 3), 1 },
			Priority { static_cast<u16>(bg3_cnt & 3), 2 },
			Priority { static_cast<u16>(bg4_cnt & 3), 3 }
		};

		u32 total_bgs = 0;

		if (layer_enabled_global[0])
			priorities[total_bgs++] = Priority{ static_cast<u16>(bg1_cnt & 3), 0 };

		if (layer_enabled_global[1])
			priorities[total_bgs++] = Priority{ static_cast<u16>(bg2_cnt & 3), 1 };

		if (layer_enabled_global[2])
			priorities[total_bgs++] = Priority{ static_cast<u16>(bg3_cnt & 3), 2 };

		if (layer_enabled_global[3])
			priorities[total_bgs++] = Priority{ static_cast<u16>(bg4_cnt & 3), 3 };

		std::sort(
			std::begin(priorities),
			std::begin(priorities) + total_bgs,
			[&layer_enabled_global](auto const& p1, auto const& p2) { 
				return p1.priority < p2.priority; 
			});

		u16 color_special_effects_reg = ReadRegister16(0x50 / 2);
		u16 curr_effect = (color_special_effects_reg >> 6) & 3;
		u16 first_target = color_special_effects_reg & 0x3F;
		u16 second_target = (color_special_effects_reg >> 8) & 0x3F;

		u16 backdrop = *reinterpret_cast<u16*>(m_palette_ram);

		u16 bldalpha = ReadRegister16(0x52 / 2);

		u16 eva = std::min(bldalpha & 0x1F, 16);
		u16 evb = std::min((bldalpha >> 8) & 0x1F, 16);

		for (u16 x = 0; x < 240; x++) {
			u8 candidate_layer_index = 0;
			u8 window_id = get_current_window_id(x);

			bool candidate_found = false;

			u8 curr_index = 0;

			while (!candidate_found && curr_index < total_bgs) {
				u8 curr_layer = priorities[curr_index].layer;

				candidate_found = !!backgrounds[curr_layer][x].palette_id
					&& (window_id == 3 || windows[window_id].layer_enable[curr_layer]);

				if (!candidate_found)
					curr_index++;
			}

			u8 layer = candidate_found ? priorities[curr_index].layer : 0;

			if (backgrounds[4][x].is_present && backgrounds[4][x].palette_id
				&& (window_id == 3 || windows[window_id].layer_enable[4])
				&& layer_enabled_global[4]) {
				if (!candidate_found || priorities[curr_index].priority >= backgrounds[4][x].priority) {
					candidate_found = true;
					layer = 4;
				}
			}

			if (candidate_found) {
				merged[x] = backgrounds[layer][x];
			}
			else {
				merged[x].color = backdrop;
				merged[x].palette_id = 1;
				layer = 5;
			}

			if ((window_id == 3 || windows[window_id].enable_special_effects)
				&& (CHECK_BIT(first_target, layer) || merged[x].is_bld_enabled)) {
				u16 special_effect_select = curr_effect;
				u16 real_effect_select = curr_effect;

				if (merged[x].is_bld_enabled)
					special_effect_select = 1;

				switch (special_effect_select)
				{
				case 0x1: {
					//Try to select second target
					second_target = CLEAR_BIT(second_target, layer);

					//Selected layer is
					//backdrop, we 
					//do not have other
					//layers to select
					if (layer == 5)
						break;

					u8 index = 0;

					bool blend_fail = false;

					if (layer < 4 && backgrounds[4][x].is_present && backgrounds[4][x].priority
						>= priorities[curr_index].priority 
						&& (window_id == 3 || windows[window_id].layer_enable[4])
						&& layer_enabled_global[4]
						&& CHECK_BIT(second_target, 4)) {
						index = 4;
					}
					else {
						index = layer == 4 ? 0 : curr_index + 1;
						u16 top_priority = layer == 4 ? merged[x].priority : priorities[curr_index].priority;

						while (index < total_bgs) {
							u8 curr_layer = priorities[index].layer;

							if (priorities[index].priority >= top_priority) {
								if ((window_id == 3 || windows[window_id].layer_enable[4])
									&& layer_enabled_global[layer]
									&& CHECK_BIT(second_target, curr_layer)
									&& backgrounds[curr_layer][x].palette_id) {
									break;
								}

								index++;
							}
							else {
								blend_fail = true;
								break;
							}
						}

						if (index == total_bgs)
							index = 6; //Layer in BGs not found,
									   //set invalid index
					}

					if (blend_fail)
						break;

					if (index == 6) {
						if (CHECK_BIT(second_target, 5)) {
							index = 5;
						}
						else if (merged[x].is_bld_enabled)
							goto brightness;
						else
							break;
					}
					
					u8 second_target_layer = index < 4 ? priorities[index].layer : index;

					if (!CHECK_BIT(second_target, second_target_layer))
						break;

					Pixel top_pixel = merged[x];
					Pixel lower_pixel = Pixel{};

					if (second_target_layer == 5) {
						lower_pixel.palette_id = 1;
						lower_pixel.color = backdrop;
					}
					else {
						lower_pixel = backgrounds[second_target_layer][x];
					}

					if (lower_pixel.palette_id) {
						//Blend
						u16 color = top_pixel.color;
						u16 color2 = lower_pixel.color;

						u8 r = color & 0x1F;
						u8 g = (color >> 5) & 0x1F;
						u8 b = (color >> 10) & 0x1F;

						u8 r2 = color2 & 0x1F;
						u8 g2 = (color2 >> 5) & 0x1F;
						u8 b2 = (color2 >> 10) & 0x1F;

						//I = MIN ( 31, I1st*EVA + I2nd*EVB )
						r = std::min((r * eva + r2 * evb + 8) >> 4, 31);
						g = std::min((g * eva + g2 * evb + 8) >> 4, 31);
						b = std::min((b * eva + b2 * evb + 8) >> 4, 31);

						merged[x].color = r | (g << 5) | (b << 10);
					}
					else if (merged[x].is_bld_enabled) {
						goto brightness;
					}
				}
					break;
				case 0x2:
				case 0x3: {
				brightness:
					//Brightness decrease/increase
					u16 evy = std::min( ReadRegister16(0x54 / 2) & 0x1F, 16);

					u16 color = merged[x].color;

					u8 r = color & 0x1F;
					u8 g = (color >> 5) & 0x1F;
					u8 b = (color >> 10) & 0x1F;

					if (real_effect_select == 0x2) {
						r += ((31 - r) * evy + 8) >> 4;
						g += ((31 - g) * evy + 8) >> 4;
						b += ((31 - b) * evy + 8) >> 4;
					}
					else if(real_effect_select == 0x3) {
						r -= (r * evy + 7) >> 4;
						g -= (g * evy + 7) >> 4;
						b -= (b * evy + 7) >> 4;
					}

					merged[x].color = r | (g << 5) | (b << 10);
				}
					break;
				default:
					break;
				}
			}
		}

		return merged;
	}

	std::array<Pixel, 240> PPU::MergeBitmap(
		std::array<Pixel, 240> const& bg,
		std::array<Pixel, 240> const& sprites
	) {
		bool mosaic = (ReadRegister16(0xC / 2) >> 6) & 1;

		if (mosaic)
			error::DebugBreak();

		std::array<Pixel, 240> merged{};

		u16 bg2_cnt = ReadRegister16(0xC / 2);

		u8 mode = m_ctx.m_control & 0x7;

		//Priority goes from 0 to 3 with 0 highest
		//Between same priority, lower bg id wins

		//Extract bg with highest priority

		//////////////////

		bool layer_enabled_global[2] = {
			(m_ctx.m_control >> 10) & 1,
			(m_ctx.m_control >> 12) & 1
		};

		bool win0_en = (m_ctx.m_control >> 13) & 1;
		bool win1_en = (m_ctx.m_control >> 14) & 1;

		bool win_enabled = win0_en || win1_en;

		u16 winin_cnt = ReadRegister16(0x48 / 2);
		u16 winout_cnt = ReadRegister16(0x4A / 2);

		bool win0_obj_enable = (winin_cnt >> 4) & 1;
		bool win1_obj_enable = (winin_cnt >> 12) & 1;
		bool winout_obj_enable = (winout_cnt >> 4) & 1;

		u16 win0_h = ReadRegister16(0x40 / 2);
		u16 win1_h = ReadRegister16(0x42 / 2);

		u16 win0_v = ReadRegister16(0x44 / 2);
		u16 win1_v = ReadRegister16(0x46 / 2);

		u16 win0_left = (win0_h >> 8) & 0xFF;
		u16 win0_right = (win0_h & 0xFF) - 1;

		u16 win0_top = (win0_v >> 8) & 0xFF;
		u16 win0_bottom = (win0_v & 0xFF) - 1;

		u16 win1_left = (win1_h >> 8) & 0xFF;
		u16 win1_right = (win1_h & 0xFF) - 1;

		u16 win1_top = (win1_v >> 8) & 0xFF;
		u16 win1_bottom = (win1_v & 0xFF) - 1;

		if (win0_right == 0xFFFF)
			win0_right = 0;

		if (win0_bottom == 0xFFFF)
			win0_bottom = 0;

		if (win1_right == 0xFFFF)
			win1_right = 0;

		if (win1_bottom == 0xFFFF)
			win1_bottom = 0;

		struct WindowInfo {
			bool enabled;
			u16 left;
			u16 right;
			u16 top;
			u16 bottom;
			int layer_enable[2];
			int enable_special_effects;
		};

		WindowInfo windows[4] = {
			WindowInfo { win0_en, win0_left, win0_right, win0_top, win0_bottom, {
				(winin_cnt >> 2) & 1,
				win0_obj_enable
			}, (winin_cnt >> 5) & 1 },
			WindowInfo { win1_en, win1_left, win1_right, win1_top, win1_bottom, {
				(winin_cnt >> 10) & 1,
				win1_obj_enable
			}, (winin_cnt >> 13) & 1 },
			WindowInfo { win_enabled, 0, 0, 0, 0, {
				(winout_cnt >> 2) & 1,
				winout_obj_enable
			}, (winout_cnt >> 5) & 1 },
			WindowInfo { true, 0, 0, 0, 0, {
			}, true }
		};

		u16 curr_line = m_ctx.m_vcount;

		bool window0_line = curr_line >= windows[0].top && curr_line < windows[0].bottom && windows[0].enabled;
		bool window1_line = curr_line >= windows[1].top && curr_line < windows[1].bottom && windows[1].enabled;

		bool curr_line_has_window = window0_line || window1_line;

		auto get_current_window_id = [&](u16 x_pos) {
			if (!curr_line_has_window) {
				if (windows[2].enabled)
					return 2;
				return 3;
			}

			if (x_pos >= windows[0].left && x_pos < windows[0].right && window0_line)
				return 0;
			else if (x_pos >= windows[1].left && x_pos < windows[1].right && window1_line)
				return 1;
			return 2;
		};

		u16 color_special_effects_reg = ReadRegister16(0x50 / 2);
		u16 curr_effect = (color_special_effects_reg >> 6) & 3;
		u16 first_target = color_special_effects_reg & 0x3F;
		u16 second_target = (color_special_effects_reg >> 8) & 0x3F;

		u16 backdrop = *reinterpret_cast<u16*>(m_palette_ram);

		u16 bldalpha = ReadRegister16(0x52 / 2);

		u16 eva = std::min(bldalpha & 0x1F, 16);
		u16 evb = std::min((bldalpha >> 8) & 0x1F, 16);

		for (u16 x = 0; x < 240; x++) {
			u8 window_id = get_current_window_id(x);
			bool candidate_found = false;

			u8 layer = 2;

			if (sprites[x].is_present && sprites[x].palette_id
				&& (window_id == 3 || windows[window_id].layer_enable[1])
				&& layer_enabled_global[1]) {
				candidate_found = true;
				merged[x] = sprites[x];
				layer = 1;
			}
			else if (bg[x].palette_id
				&& (window_id == 3 || windows[window_id].layer_enable[0])
				&& layer_enabled_global[0]) {
				candidate_found = true;
				merged[x] = bg[x];
				layer = 0;
			}

			if (!candidate_found) {
				merged[x].color = backdrop;
				merged[x].palette_id = 1;
			}

			if ((window_id == 3 || windows[window_id].enable_special_effects)
				&& CHECK_BIT(first_target, layer)) {
				u16 special_effect_select = curr_effect;
				u16 real_effect_select = curr_effect;

				if (merged[x].is_bld_enabled)
					special_effect_select = 1;

				switch (special_effect_select)
				{
				case 0x1: {
					//Try to select second target
					second_target = CLEAR_BIT(second_target, layer);

					//Selected layer is
					//backdrop, we 
					//do not have other
					//layers to select
					if (layer == 2)
						break;

					u8 index = 5;

					if (layer == 1
						&& (window_id == 3 || windows[window_id].layer_enable[0])
						&& layer_enabled_global[0]
						&& CHECK_BIT(second_target, 2)) {
						index = 2;
					}
					else if(
						(window_id == 3 || windows[window_id].layer_enable[1])
						&& layer_enabled_global[1]
						&& CHECK_BIT(second_target, 4)
					) {
						index = 4;
					}

					if (index == 5) {
						if (CHECK_BIT(second_target, 5)) {
							index = 5;
						}
						else if (merged[x].is_bld_enabled)
							goto brightness;
						else
							break;
					}

					u8 second_target_layer = index;

					if (!CHECK_BIT(second_target, second_target_layer))
						break;

					Pixel top_pixel = merged[x];
					Pixel lower_pixel = Pixel{};

					if (second_target_layer == 5) {
						lower_pixel.palette_id = 1;
						lower_pixel.color = backdrop;
					}
					else {
						lower_pixel = second_target_layer == 2 ? bg[x] : sprites[x];
					}

					if (lower_pixel.palette_id) {
						//Blend
						u16 color = top_pixel.color;
						u16 color2 = lower_pixel.color;

						u8 r = color & 0x1F;
						u8 g = (color >> 5) & 0x1F;
						u8 b = (color >> 10) & 0x1F;

						u8 r2 = color2 & 0x1F;
						u8 g2 = (color2 >> 5) & 0x1F;
						u8 b2 = (color2 >> 10) & 0x1F;

						//I = MIN ( 31, I1st*EVA + I2nd*EVB )
						r = std::min((r * eva + r2 * evb + 8) >> 4, 31);
						g = std::min((g * eva + g2 * evb + 8) >> 4, 31);
						b = std::min((b * eva + b2 * evb + 8) >> 4, 31);

						merged[x].color = r | (g << 5) | (b << 10);
					}
					else if (merged[x].is_bld_enabled) {
						goto brightness;
					}
				}
						break;
				case 0x2:
				case 0x3: {
				brightness:
					//Brightness decrease/increase
					u16 evy = ReadRegister16(0x54 / 2) & 0x1F;

					u16 color = merged[x].color;

					u8 r = color & 0x1F;
					u8 g = (color >> 5) & 0x1F;
					u8 b = (color >> 10) & 0x1F;

					if (real_effect_select == 0x2) {
						r += ((31 - r) * evy + 8) >> 4;
						g += ((31 - g) * evy + 8) >> 4;
						b += ((31 - b) * evy + 8) >> 4;
					}
					else if (real_effect_select == 0x3) {
						r -= (r * evy + 7) >> 4;
						g -= (g * evy + 7) >> 4;
						b -= (b * evy + 7) >> 4;
					}

					merged[x].color = r | (g << 5) | (b << 10);
				}
						break;
				default:
					break;
				}
			}
		}

		return merged;
	}
}