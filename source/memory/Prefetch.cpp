#include "../../memory/Bus.hpp"

#include "../../gamepack/GamePack.hpp"

namespace GBA::memory {
	template <>
	u16 Bus::Prefetch<u16>(u32 address, bool code, MEMORY_RANGE region) {
		u32 addr_low = address & 0x00FFFFFF;

		if (!code || !m_enable_prefetch) {
			StopPrefetch();

			if (region == MEMORY_RANGE::ROM_REG_1)
				m_time.PushCycles<MEMORY_RANGE::ROM_REG_1, 2>();
			else if (region == MEMORY_RANGE::ROM_REG_2)
				m_time.PushCycles<MEMORY_RANGE::ROM_REG_2, 2>();
			else 
				m_time.PushCycles<MEMORY_RANGE::ROM_REG_3, 2>();

			return m_pack->Read(addr_low);
		}

		return m_pack->Read(addr_low);
	}

	template <>
	u32 Bus::Prefetch<u32>(u32 address, bool code, MEMORY_RANGE region) {
		u32 addr_low = address & 0x00FFFFFF;

		u32 value = m_pack->Read(addr_low);
		value |= (m_pack->Read(addr_low + 2) << 16);

		if (!code || !m_enable_prefetch) {
			StopPrefetch();

			if (region == MEMORY_RANGE::ROM_REG_1)
				m_time.PushCycles<MEMORY_RANGE::ROM_REG_1, 4>();
			else if (region == MEMORY_RANGE::ROM_REG_2)
				m_time.PushCycles<MEMORY_RANGE::ROM_REG_2, 4>();
			else
				m_time.PushCycles<MEMORY_RANGE::ROM_REG_3, 4>();

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