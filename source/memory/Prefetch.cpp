#include "../../memory/Bus.hpp"

#include "../../gamepack/GamePack.hpp"

namespace GBA::memory {
	template <>
	u16 Bus::Prefetch<u16>(u32 address, bool code, MEMORY_RANGE region, u32& cycles) {
		u32 addr_low = address & 0x00FFFFFF;
		addr_low &= ~1;

		addr_low += (
			region == MEMORY_RANGE::ROM_REG_1_SECOND ||
			region == MEMORY_RANGE::ROM_REG_2_SECOND ||
			region == MEMORY_RANGE::ROM_REG_3_SECOND
			)
			? 0x01000000 : 0x0;

		if (!code || !m_enable_prefetch) {
			StopPrefetch();

			Access acc = m_time.access;

			if ((addr_low & 0x1ffff) == 0)
				acc = Access::NonSeq;

			if (region == MEMORY_RANGE::ROM_REG_1 || region == MEMORY_RANGE::ROM_REG_1_SECOND)
				cycles = m_time.PushCycles<MEMORY_RANGE::ROM_REG_1, 2>(acc);
			else if (region == MEMORY_RANGE::ROM_REG_2 || region == MEMORY_RANGE::ROM_REG_2_SECOND)
				cycles = m_time.PushCycles<MEMORY_RANGE::ROM_REG_2, 2>(acc);
			else 
				cycles = m_time.PushCycles<MEMORY_RANGE::ROM_REG_3, 2>(acc);

			return m_pack->Read(addr_low, (u8)region - 8);
		}

		return m_pack->Read(addr_low, (u8)region - 8);
	}

	template <>
	u32 Bus::Prefetch<u32>(u32 address, bool code, MEMORY_RANGE region, u32& cycles) {
		u32 addr_low = address & 0x00FFFFFF;
		addr_low &= ~3;

		addr_low += (
			region == MEMORY_RANGE::ROM_REG_1_SECOND ||
			region == MEMORY_RANGE::ROM_REG_2_SECOND ||
			region == MEMORY_RANGE::ROM_REG_3_SECOND
			)
			? 0x01000000 : 0x0;

		u32 value = m_pack->Read(addr_low, (u8)region - 8);
		value |= (m_pack->Read(addr_low + 2, (u8)region - 8) << 16);

		if (!code || !m_enable_prefetch) {
			StopPrefetch();

			Access acc = m_time.access;

			if ((addr_low & 0x1ffff) == 0)
				acc = Access::NonSeq;

			if (region == MEMORY_RANGE::ROM_REG_1 || region == MEMORY_RANGE::ROM_REG_1_SECOND)
				cycles = m_time.PushCycles<MEMORY_RANGE::ROM_REG_1, 4>(acc);
			else if (region == MEMORY_RANGE::ROM_REG_2 || region == MEMORY_RANGE::ROM_REG_2_SECOND)
				cycles = m_time.PushCycles<MEMORY_RANGE::ROM_REG_2, 4>(acc);
			else
				cycles = m_time.PushCycles<MEMORY_RANGE::ROM_REG_3, 4>(acc);

			return value;
		}

		return value;
	}

	void Bus::StopPrefetch() {
		if (m_prefetch.active) {
			m_prefetch.active = false;
			m_prefetch.current_cycles = 0;
		}
	}

	void Bus::StartPrefetch(u32 start_address, MEMORY_RANGE region) {
		if (!m_prefetch.active) {
			m_prefetch.active = true;
			m_prefetch.address = start_address;
			m_prefetch.duty = m_time.GetSequentialAccess(region);
			m_prefetch.current_cycles = 0;
		}
	}
}