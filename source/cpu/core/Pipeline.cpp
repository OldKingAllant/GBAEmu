#include "../../../cpu/core/Pipeline.hpp"

namespace GBA::cpu {
	Pipeline::Pipeline(memory::Bus* bus) :
		m_fetch_pc(0x0), m_fetched(0x0),
		m_decoded(0x0), m_bus(bus)
	{}

	u32 Pipeline::GetFetchPC() const {
		return m_fetch_pc;
	}
}