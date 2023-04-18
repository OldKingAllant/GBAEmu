#include "../../../cpu/core/Pipeline.hpp"

namespace GBA::cpu {
	Pipeline::Pipeline() :
		m_fetch_pc(0x0), m_fetched(0x0),
		m_decoded(0x0), m_bus(nullptr)
	{}

	u32 Pipeline::GetFetchPC() const {
		return m_fetch_pc;
	}

	u32 Pipeline::GetFetched() const {
		return m_fetched;
	}

	u32 Pipeline::GetDecoded() const {
		return m_decoded;
	}
}