#pragma once

#include "THUMB_Implementation.hpp"
#include "../../common/static_for.hpp"
#include <array>

namespace GBA::cpu::thumb::detail {
	constexpr THUMBInstructionType DecodeThumbConstexpr(u16 opcode) {
		for (u8 i = 0; i < 19; i++) {
			u16 masked = opcode & THUMB_MASKS[i];

			if (masked == THUMB_VALUES[i]) {
				if (i == 0 && ((opcode >> 11) & 0x3) == 0x3)
					return THUMBInstructionType::FORMAT_02;
				else if (i == 15 && ((opcode >> 8) & 0xF) == 0xF)
					return THUMBInstructionType::FORMAT_17;

				return (THUMBInstructionType)i;
			}
		}

		return THUMBInstructionType::UNDEFINED;
	}

	constexpr std::array<THUMBInstructionType, 1024> GenerateThumbLut() {
		std::array<THUMBInstructionType, 1024> lut{};

		common::static_for<0, 1024>([&lut](u16 index, u32 dummy) {
			THUMBInstructionType type = DecodeThumbConstexpr(index << 6);

			lut[index] = type;

			return dummy + 1;
		}, 0);

		return lut;
	}

	constexpr std::array<THUMBInstructionType, 1024> thumb_lookup_table = GenerateThumbLut();
}