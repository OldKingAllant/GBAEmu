#pragma once

void CalculateMosaicBG(GBA::common::i32& x, GBA::common::i32& y);

std::array<Pixel, 240> ProcessNormalBackground(int bg_id, int lcd_y);
std::array<Pixel, 240> ProcessAffineBackground(int bg_id, int lcd_y);

std::array<Pixel, 240> MergeBackrounds(
	std::array<Pixel, 240> const& bg1, 
	std::array<Pixel, 240> const& bg2,
	std::array<Pixel, 240> const& bg3,
	std::array<Pixel, 240> const& bg4,
	std::array<Pixel, 240> const& sprites
);