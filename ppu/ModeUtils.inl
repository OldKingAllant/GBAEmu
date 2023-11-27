#pragma once

void CalculateMosaicBG(GBA::common::i32& x, GBA::common::i32& y);

void ProcessNormalBackground(int bg_id, int lcd_y);
void ProcessAffineBackground(int bg_id, int lcd_y);

std::array<Pixel, 240> MergeBackrounds();

std::array<Pixel, 240> MergeBitmap(
	std::array<Pixel, 240> const& bg2,
	std::array<Pixel, 240> const& sprites
);