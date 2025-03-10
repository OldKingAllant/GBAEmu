#include "../../../cpu/core/InterpreterCache.hpp"

#include "../../../memory/Bus.hpp"
#include "../../../common/Error.hpp"

#include <fmt/format.h>

namespace GBA::cpu {
	static constexpr u32 DEFAULT_BLOCK_LEN = 64;
	static constexpr u32 DEFAULT_REGION_SZ = 256;

	InterpreterCache::InterpreterCache() :
		m_block_len{DEFAULT_BLOCK_LEN}, 
		m_region_len{DEFAULT_REGION_SZ},
		m_region_shift{}, m_bios_cache{},
		m_rom_cache{}, m_iwram_cache{},
		m_curr_block{nullptr}
	{}

	void InterpreterCache::SetBlocksLen(u32 block_len) {
		m_block_len = block_len;
	}

	void InterpreterCache::SetRegionLen(u32 region_sz) {
		if ((region_sz & (region_sz - 1)) > 0) {
			fmt::println("[INTERPRETER] Invalid region size, must be power of two!");
			fmt::println("              Using previous size {}", m_region_len);
		}

		m_region_len = region_sz;
	}

	void InterpreterCache::Init() {
		using namespace memory;

		const auto ARM_BLOCK_SIZE   = m_block_len << 2;
		const auto THUMB_BLOCK_SIZE = m_block_len << 1;

		if (ARM_BLOCK_SIZE > m_region_len || THUMB_BLOCK_SIZE > m_region_len) {
			fmt::println("[INTERPRETER] Block size x2 or x4 > region size");
			m_block_len  = DEFAULT_BLOCK_LEN;
			m_region_len = DEFAULT_REGION_SZ;
		}

		m_region_shift = std::countr_zero(m_region_len);
		
		u32 num_bios_regions  = (REGIONS_LEN[u8(MEMORY_RANGE::BIOS)] + 1) >> m_region_shift;
		u32 num_rom_regions   = uint32_t(Bus::ROM_REGION_SIZE >> m_region_shift);
		u32 num_iwram_regions = (REGIONS_LEN[u8(MEMORY_RANGE::IWRAM)] + 1) >> m_region_shift;

		m_bios_cache  = std::make_unique<BlockList[]>(num_bios_regions);
		m_rom_cache   = std::make_unique<BlockList[]>(num_rom_regions);
		m_iwram_cache = std::make_unique<BlockList[]>(num_iwram_regions);
	}

	bool InterpreterCache::IsCacheable(u32 address) const {
		using namespace memory;

		auto region = MEMORY_RANGE(address >> 24);

		switch (region)
		{
		case MEMORY_RANGE::BIOS:
		case MEMORY_RANGE::IWRAM:
		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_1_SECOND:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_2_SECOND:
		case MEMORY_RANGE::ROM_REG_3:
		case MEMORY_RANGE::ROM_REG_3_SECOND:
			return true;
		default:
			break;
		}

		return false;
	}

	BlockList* InterpreterCache::GetBlockList(u32 region_u32, u32 page_in_region) const {
		using namespace memory;

		auto region = MEMORY_RANGE(region_u32);

		BlockList* list{ nullptr };

		switch (region)
		{
		case MEMORY_RANGE::BIOS:
			list = &m_bios_cache[page_in_region];
			break;
		case MEMORY_RANGE::IWRAM:
			list = &m_iwram_cache[page_in_region];
			break;
		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_1_SECOND:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_2_SECOND:
		case MEMORY_RANGE::ROM_REG_3:
		case MEMORY_RANGE::ROM_REG_3_SECOND:
			list = &m_rom_cache[page_in_region];
			break;
		default:
			error::Unreachable();
			break;
		}

		return list;
	}

	Block** InterpreterCache::GetBlock(u32 address) {
		using namespace memory;

		if (!IsCacheable(address))
			return nullptr;

		auto region         = MEMORY_RANGE(address >> 24);
		auto region_offset  = (address & REGIONS_LEN[u8(region)]);
		auto page_in_region = region_offset >> m_region_shift;
		auto page_offset    = region_offset & (m_region_len - 1);

		BlockList* list = GetBlockList(u32(region), page_in_region);

		m_curr_block = nullptr;

		if (list->blocks.empty()) {
			return &m_curr_block;
		}

		auto iter = list->blocks.begin();
		auto end  = list->blocks.end();
		
		while (iter != end) {
			if (iter->base_address == page_offset) {
				m_curr_block = &*iter;
				break;
			}

			++iter;
		}

		return &m_curr_block;
	}

	void InterpreterCache::AddBlock(u32 address, Block&& new_block) {
		using namespace memory;

		//fmt::println("[INTERPRETER] ADDING BLOCK AT {:#010x}", address);

		auto region			= MEMORY_RANGE(address >> 24);
		auto region_offset  = (address & REGIONS_LEN[u8(region)]);
		auto page_in_region = region_offset >> m_region_shift;
		auto page_offset	= region_offset & (m_region_len - 1);

		BlockList* list = GetBlockList(u32(region), page_in_region);

		new_block.base_address = page_offset;
		new_block.region       = page_in_region;
		new_block.mem_range	   = u32(region);

		list->blocks.push_back(std::move(new_block));
	}

	void InterpreterCache::Invalidate(u32 address, u32 write_size) {
		using namespace memory;

		auto region         = MEMORY_RANGE(address >> 24);

		if (region != MEMORY_RANGE::IWRAM)
			return;

		auto region_offset  = (address & REGIONS_LEN[u8(region)]);
		auto page_in_region = region_offset >> m_region_shift;
		auto page_offset    = region_offset & (m_region_len - 1);
		
		auto first_region  = GetRegionFromAddress(address);
		auto second_region = GetRegionFromAddress(address + write_size - 1);
		
		if (first_region != second_region) [[unlikely]] {
			fmt::println("[INTERPRETER] Write crosses region boundary");
			error::DebugBreak();
		}
		
		BlockList* list = GetBlockList(u32(region), page_in_region);
		
		if (m_curr_block) [[likely]] {
			if (m_curr_block->region == first_region &&
				m_curr_block->mem_range == u32(region)) [[unlikely]] {
				fmt::println("[INTERPRETER] Invalidated current block!");
				m_curr_block = nullptr;
			}	
		}

		if (!list->blocks.empty()) {
			//fmt::println("[INTERPRETER] Invalidating {} blocks",
			//	list->blocks.size());
			//Invalidate all blocks
			list->blocks.clear();
		}
	}

	u32 InterpreterCache::GetRegionFromAddress(u32 address) const {
		using namespace memory;

		auto region = MEMORY_RANGE(address >> 24);
		auto region_offset = (address & REGIONS_LEN[u8(region)]);
		auto page_in_region = region_offset >> m_region_shift;

		return page_in_region;
	}
}