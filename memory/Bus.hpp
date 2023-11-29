#pragma once

#include "../common/Defs.hpp"
#include "../common/Error.hpp"

#include "Timing.hpp"

#include "MMIO.hpp"
#include "EventScheduler.hpp"
#include "../ppu/PPU.hpp"

#include "DMAFireType.hpp"

#include "../gamepack/GamePack.hpp"

#include "../common/Logger.hpp"

#include "../memory/Timers.hpp"

namespace GBA::gamepack {
	class GamePack;
}

namespace GBA::cpu {
	class ARM7TDI;
}

namespace GBA::ppu {
	class PPU;
}

namespace GBA::timers {
	class TimerChain;
}

namespace GBA::memory {
	class MMIO;
	class DMA;

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
		0xFFFFFF,
		0xFFFFFF,
		0xFFFFFF,
		0xFFFF //SRAM
	};

	static constexpr u8 NUM_REGIONS = sizeof(REGIONS_LEN) / sizeof(u32);

	class Bus {
	public :
		Bus();

		void SetEventScheduler(EventScheduler* sched) {
			m_sched = sched;
		}

		template <typename Type>
		Type Prefetch(u32 address, bool code, MEMORY_RANGE region, u32& cycles);

		/*
		* Implementations are:
		* 1 byte
		* 1 halfword
		* 1 word
		*/
		template <typename Type>
		Type Read(u32 address, bool code = false) {
			MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
			u32 addr_low = address & 0x00FFFFFF;

			if constexpr (sizeof(Type) == 4)
				addr_low &= ~3;
			else if constexpr (sizeof(Type) == 2)
				addr_low &= ~1;

			constexpr const u8 type_size = sizeof(Type);

			Type return_value = 0;

			u32 num_cycles = 0;

			//Some memory regions are mirrored, others are not

			switch (region) {
			case MEMORY_RANGE::BIOS:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::BIOS, type_size>();
				
				if (addr_low > REGIONS_LEN[0])
					return_value = ReadOpenBus(address);
				else
					return_value = ReadBios<Type>(address);
				break;

			case MEMORY_RANGE::EWRAM:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::EWRAM, type_size>();

				addr_low &= REGIONS_LEN[(u8)MEMORY_RANGE::EWRAM];

				return_value = reinterpret_cast<Type*>(m_wram)[addr_low / type_size];
				break;

			case MEMORY_RANGE::IWRAM:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::IWRAM, type_size>();

				addr_low &= REGIONS_LEN[(u8)MEMORY_RANGE::IWRAM];

				return_value = reinterpret_cast<Type*>(m_iwram)[addr_low / type_size];
				break;

			case MEMORY_RANGE::IO:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::IO, type_size>();

				if (addr_low < IO_SIZE && !UNUSED_REGISTERS_MAP[addr_low]
					&& mmio->IsRegisterReadable(addr_low)) {
					m_timers->Update();
					return_value = mmio->Read<Type>(addr_low);
				}
				else {
					return_value = 0;
					//logging::Logger::Instance().LogInfo("Memory_bus",
						//" Accessing invalid/unused port {:x}", address);
				}
				break;

			case MEMORY_RANGE::PAL:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::PAL, type_size>();

				if constexpr(sizeof(Type) == 4)
					num_cycles += m_time.PushCycles<MEMORY_RANGE::PAL, type_size>();

				addr_low &= REGIONS_LEN[(u8)MEMORY_RANGE::PAL];

				return_value = m_ppu->ReadPalette<Type>(addr_low);

				break;

			case MEMORY_RANGE::VRAM:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::VRAM, type_size>();

				if constexpr (sizeof(Type) == 4)
					num_cycles += m_time.PushCycles<MEMORY_RANGE::PAL, type_size>();

				return_value = m_ppu->ReadVRAM<Type>(addr_low);
				break;

			case MEMORY_RANGE::OAM:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::OAM, type_size>();

				addr_low &= REGIONS_LEN[(u8)MEMORY_RANGE::OAM];

				return_value = m_ppu->ReadOAM<Type>(addr_low);

				break;

			case MEMORY_RANGE::ROM_REG_1:
			case MEMORY_RANGE::ROM_REG_1_SECOND:
			case MEMORY_RANGE::ROM_REG_2:
			case MEMORY_RANGE::ROM_REG_2_SECOND:
			case MEMORY_RANGE::ROM_REG_3:
			case MEMORY_RANGE::ROM_REG_3_SECOND: {
				if constexpr (sizeof(Type) == 1) {
					//addr_low &= ~1;
					u16 temp = Prefetch<u16>(address & ~1, code, region, num_cycles);
					return_value = (temp >> (8 * (addr_low % 2))) & 0xFF;
				}	
				else
					return_value = Prefetch<Type>(address, code, region, num_cycles);
			}
			break;
				

			case MEMORY_RANGE::SRAM:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::SRAM, type_size>();
				return_value = m_pack->ReadSRAM(addr_low);
				break;

			default:
				return_value = ReadOpenBus(address);
				num_cycles = 1;
				break;
			}

			m_open_bus_value = return_value;
			m_open_bus_address = address;

			m_sched->Advance(num_cycles);

			return return_value;
		}

		template <typename Type>
		void Write(u32 address, Type value) {
			MEMORY_RANGE region = (MEMORY_RANGE)(address >> 24);
			u32 addr_low = address & 0x00FFFFFF;

			if constexpr (sizeof(Type) == 4)
				addr_low &= ~3;
			else if constexpr (sizeof(Type) == 2)
				addr_low &= ~1;

			constexpr const u8 type_size = sizeof(Type);

			u32 num_cycles = 0;

			switch (region) {
			case MEMORY_RANGE::BIOS:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::BIOS, type_size>();
				break;

			case MEMORY_RANGE::EWRAM:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::EWRAM, type_size>();
				addr_low &= REGIONS_LEN[(u8)MEMORY_RANGE::EWRAM];
				reinterpret_cast<Type*>(m_wram)[addr_low / type_size] = value;
				break;

			case MEMORY_RANGE::IWRAM:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::IWRAM, type_size>();
				addr_low &= REGIONS_LEN[(u8)MEMORY_RANGE::IWRAM];
				reinterpret_cast<Type*>(m_iwram)[addr_low / type_size] = value;
				break;

			case MEMORY_RANGE::IO:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::IO, type_size>();

				if (addr_low < IO_SIZE && !UNUSED_REGISTERS_MAP[addr_low]) {
					mmio->Write<Type>(addr_low, value);
				}
				else {
					//logging::Logger::Instance().LogInfo("Memory_bus",
						//" Accessing invalid/unused port {:x}", address);
				}

				break;

			case MEMORY_RANGE::PAL:
				addr_low &= REGIONS_LEN[(u8)MEMORY_RANGE::PAL];
				
				num_cycles = m_time.PushCycles<MEMORY_RANGE::PAL, type_size>();

				if constexpr (sizeof(Type) == 4)
					num_cycles += m_time.PushCycles<MEMORY_RANGE::PAL, type_size>();

				m_ppu->WritePalette<Type>(addr_low, value);

				break;

			case MEMORY_RANGE::VRAM:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::VRAM, type_size>();

				if (addr_low >= REGIONS_LEN[(u8)MEMORY_RANGE::VRAM]) {
					logging::Logger::Instance().LogInfo("BUS", " Accessing outside of VRAM");
				}

				/*if constexpr (sizeof(Type) == 1)
					logging::Logger::Instance().LogInfo("BUS", " Writing byte size to VRAM");*/

				if constexpr (sizeof(Type) == 4)
					num_cycles += m_time.PushCycles<MEMORY_RANGE::PAL, type_size>();

				m_ppu->WriteVRAM<Type>(addr_low, value);

				break;

			case MEMORY_RANGE::OAM:
				addr_low &= REGIONS_LEN[(u8)MEMORY_RANGE::OAM];
				num_cycles = m_time.PushCycles<MEMORY_RANGE::OAM, type_size>();

				m_ppu->WriteOAM<Type>(addr_low, value);

				break;

			case MEMORY_RANGE::ROM_REG_1:
			case MEMORY_RANGE::ROM_REG_1_SECOND:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::ROM_REG_1, type_size>();
				StopPrefetch();
				m_pack->Write(address, (u16)value);
				return;

			case MEMORY_RANGE::ROM_REG_2:
			case MEMORY_RANGE::ROM_REG_2_SECOND:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::ROM_REG_2, type_size>();
				StopPrefetch();
				m_pack->Write(address, (u16)value);
				return;

			case MEMORY_RANGE::ROM_REG_3:
			case MEMORY_RANGE::ROM_REG_3_SECOND:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::ROM_REG_3, type_size>();
				StopPrefetch();
				m_pack->Write(address, (u16)value);
				break;

			case MEMORY_RANGE::SRAM:
				num_cycles = m_time.PushCycles<MEMORY_RANGE::SRAM, type_size>();
				m_pack->WriteSRAM(addr_low, value & 0xFF);
				break;
			}

			m_open_bus_value = value;
			m_open_bus_address = address;

			m_sched->Advance(num_cycles);
		}

		/*
		* Read everything without
		* changing cycle counts
		*/
		u32 DebuggerRead32(u32 address);
		u16 DebuggerRead16(u32 address);

		void ConnectGamepack(gamepack::GamePack* pack);

		
		void StopPrefetch();
		void StartPrefetch(u32 start_address, MEMORY_RANGE region);

		void InternalCycles(u32 count);

		u32 ReadBiosImpl(u32 address);

		template <typename Type>
		Type ReadBios(u32 address) {
			u32 aligned = address & ~3;

			u32 value = ReadBiosImpl(address);

			if constexpr (sizeof(Type) == 2)
				value >>= 8 * (address & 1);
			else if constexpr (sizeof(Type) == 1)
				value >>= 8 * (address & 3);

			return (Type)value;
		} 

		u32 ReadOpenBus(u32 address);

		void AttachProcessor(cpu::ARM7TDI* processor) {
			m_processor = processor;
		}

		void SetPPU(ppu::PPU* ppu) {
			m_ppu = ppu;
		}

		void SetTimers(timers::TimerChain* timers) {
			m_timers = timers;
		}

		void LoadBIOS(std::string const& location);
		void LoadBiosResetOpcode();

		MMIO* GetMMIO() {
			return mmio;
		}

		~Bus();

		inline void AddActiveDma(u8 id) {
			if (active_dmas_count >= 4) [[unlikely]]
				error::DebugBreak();

			int pos = active_dmas_count;

			while (pos > 0) {
				if (active_dmas[pos - 1] > id)
					break;

				pos--;
			}

			if (pos < 0)
				pos = 0;

			std::shift_right(active_dmas.begin() + pos, active_dmas.end(), 1);

			active_dmas[pos] = id;
			active_dmas_count++;
		}

		inline u8 GetActiveDma() const {
			return active_dmas_count ? active_dmas[((unsigned)active_dmas_count) - 1] : 4;
		}

		inline void RemoveActiveDma() {
			//Only the highest priority
			//DMA should be removed
			if(active_dmas_count)
				active_dmas_count--;
		}

		template <unsigned N>
		void SetDmas(DMA*(&dma_list)[N]) {
			static_assert(N == 4);
			std::copy(std::begin(dma_list), std::end(dma_list), dmas);
		}

		void TryTriggerDMA(DMAFireType trigger_type);

		inline void ResetHalt() {
			m_halt_cnt = 0;
		}

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

		u32 m_bios_latch;

		//I do know that this thing is horrible,
		//but it is necessary for emulating
		//BIOS protection and the open bus
		cpu::ARM7TDI* m_processor;

		u32 m_open_bus_value;
		u32 m_open_bus_address;

		MMIO* mmio;

		ppu::PPU* m_ppu;

		u8* m_bios;

		EventScheduler* m_sched;

		//DMA things
		u8 active_dmas_count;
		std::array<u8, 4> active_dmas;
		DMA* dmas[4];

		u8 m_post_boot;
		u8 m_halt_cnt;
		u32 m_mem_control;

		timers::TimerChain* m_timers;
	};
}