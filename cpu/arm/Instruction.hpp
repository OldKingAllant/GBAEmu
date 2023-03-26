#pragma once

#include "../../common/Defs.hpp"

namespace GBA::cpu::arm{
	using namespace common;

	union ARMInstruction {
		ARMInstruction(u32 data) {
			this->data = data;
		}

#pragma pack(push, 1)
		struct {
			u16 : 16;
			u8 : 8;
			bool : 1;
			u8 type : 3;
			u8 condition : 4;
		};
#pragma pack(pop)

		u32 data;
	};
}