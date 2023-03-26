#pragma once

#include "../common/Defs.hpp"

namespace GBA::gamepack {
	class GamePack;
}

namespace GBA::memory {
	using namespace common;

	static constexpr u32 REGIONS_LEN[] = {
		0x3FFF, //BIOS
		0xFFFFFF, //UNUSED 1
		0x3FFFF, //WRAM
		0x7FFF, //IWRAM
		0x3FE, //I/O
		0x3FF, //Palette RAM
		0x17FFF, //VRAM
		0x3FF, //OAM
		0xFFFFFF,
		0xFFFFFF,
		0xFFFFFF,
		0xFFFF //SRAM
	};

	static constexpr u8 NUM_REGIONS = sizeof(REGIONS_LEN) / sizeof(u32);

	enum class MEMORY_RANGE : u8 {
		BIOS = 0x00,
		EWRAM = 0x02,
		IWRAM = 0x03,
		IO = 0x04,
		PAL = 0x05,
		VRAM = 0x06,
		OAM = 0x07,
		ROM_REG_1 = 0x08,
		ROM_REG_2 = 0x0A,
		ROM_REG_3 = 0x0C,
		SRAM = 0x0E
	};

	class Bus {
	public :
		Bus();

		/*
		* Implementations are:
		* 1 byte
		* 1 halfword
		* 1 word
		*/
		template <typename Type>
		Type Read(u32 address) const;

		template <typename Type>
		void Write(u32 address, Type value);

		void ConnectGamepack(gamepack::GamePack* pack);

	private :
		gamepack::GamePack* m_pack;
	};
}