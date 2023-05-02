#pragma once

#include "../common/Defs.hpp"

#include "Timing.hpp"

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
		Type Read(u32 address, bool code = false);

		template <typename Type>
		void Write(u32 address, Type value);

		/*
		* Read everything without
		* changing cycle counts
		*/
		u32 DebuggerRead32(u32 address);
		u16 DebuggerRead16(u32 address);

		void ConnectGamepack(gamepack::GamePack* pack);

		template <typename Type>
		Type Prefetch(u32 address, bool code, MEMORY_RANGE region);
		void StopPrefetch();
		void StartPrefetch(u32 start_address, MEMORY_RANGE region);

		void InternalCycles(u32 count);

		~Bus();

		TimeManager m_time;

	private :
		gamepack::GamePack* m_pack;

		u8* m_wram;
		u8* m_iwram;

		struct Prefetch {
			bool active;
			u32 address;
			u8 duty;
			int current_cycles;
		} m_prefetch;

		bool m_enable_prefetch;
	};
}