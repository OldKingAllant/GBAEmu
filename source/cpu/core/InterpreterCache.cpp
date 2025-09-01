#include "../../../cpu/core/InterpreterCache.hpp"

#include "../../../memory/Bus.hpp"
#include "../../../common/Error.hpp"

#include <fmt/format.h>

namespace GBA::cpu {
	static constexpr u32 DEFAULT_BLOCK_LEN = 64;
	static constexpr u32 DEFAULT_REGION_SZ = 256;

	InterpreterCache::InterpreterCache() :
		m_block_len{DEFAULT_BLOCK_LEN}, 
		m_page_len{DEFAULT_REGION_SZ},
		m_page_shift{}, m_bios_cache{},
		m_rom_cache{}, m_iwram_cache{},
		m_iwram_page_blocks{},
		m_curr_block{nullptr},
		m_was_init{false}
	{}

	void InterpreterCache::SetBlocksLen(u32 block_len) {
		m_block_len = block_len;
	}

	void InterpreterCache::SetPageLen(u32 region_sz) {
		if ((region_sz & (region_sz - 1)) > 0) {
			fmt::print("[INTERPRETER] Invalid region size, must be power of two!\n");
			fmt::print("              Using previous size {}\n", m_page_len);
		}

		m_page_len = region_sz;
	}

	void InterpreterCache::Init() {
		using namespace memory;

		const auto ARM_BLOCK_SIZE   = m_block_len << 2;
		const auto THUMB_BLOCK_SIZE = m_block_len << 1;

		if (ARM_BLOCK_SIZE > m_page_len || THUMB_BLOCK_SIZE > m_page_len) {
			fmt::print("[INTERPRETER] Block size x2 or x4 > region size\n");
			m_block_len  = DEFAULT_BLOCK_LEN;
			m_page_len = DEFAULT_REGION_SZ;
		}

		//Get number of right shifts we need to perform
		// on and address to get its page number
		m_page_shift = std::countr_zero(m_page_len);

		constexpr uint64_t BIOS_SIZE = REGIONS_LEN[u8(MEMORY_RANGE::BIOS)] + 1ULL;
		constexpr uint64_t ROM_SIZE  = Bus::ROM_REGION_SIZE;
		constexpr uint64_t IRAM_SIZE = REGIONS_LEN[u8(MEMORY_RANGE::IWRAM)] + 1ULL;
		
		//Total number of pages in which the IWRAM is divided
		const u32 num_iwram_pages = (REGIONS_LEN[u8(MEMORY_RANGE::IWRAM)] + 1) >> m_page_shift;

		//A THUMB instructions is 2 bytes,
		//so we divide the region sizes by 2
		m_bios_cache.resize(BIOS_SIZE >> 1);
		m_rom_cache.resize(ROM_SIZE >> 1);
		m_iwram_cache.resize(IRAM_SIZE >> 1);

		m_iwram_page_blocks.resize(num_iwram_pages);

		m_was_init = true;
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

	std::vector<std::unique_ptr<Block>>* InterpreterCache::GetBlockRegion(u32 address)
	{
		using namespace memory;

		auto region = MEMORY_RANGE(address >> 24);

		switch (region)
		{
		case MEMORY_RANGE::BIOS:
			return &m_bios_cache;
		case MEMORY_RANGE::IWRAM:
			return &m_iwram_cache;
		case MEMORY_RANGE::ROM_REG_1:
		case MEMORY_RANGE::ROM_REG_1_SECOND:
		case MEMORY_RANGE::ROM_REG_2:
		case MEMORY_RANGE::ROM_REG_2_SECOND:
		case MEMORY_RANGE::ROM_REG_3:
		case MEMORY_RANGE::ROM_REG_3_SECOND:
			return &m_rom_cache;
			break;
		default:
			error::Unreachable();
			break;
		}
		
		return nullptr;
	}

	Block** InterpreterCache::GetBlock(u32 address) {
		using namespace memory;

		m_curr_block = nullptr;

		//If we are not in cacheable region, 
		//return
		if (!IsCacheable(address))
			return nullptr;

		//Get the current region and the offset inside
		//said region
		auto region         = MEMORY_RANGE(address >> 24);
		auto region_offset  = (address & REGIONS_LEN[u8(region)]) >> 1;

		//Get region cache
		auto region_ptr = GetBlockRegion(address);

		//Fast block retrieval, O(1)
		auto& block_ptr = (*region_ptr)[region_offset];
		if (!block_ptr) {
			return &m_curr_block;
		}

		//Get the pointer directly
		m_curr_block = block_ptr.get();

		return &m_curr_block;
	}

	void InterpreterCache::AddBlock(u32 address, Block&& new_block) {
		using namespace memory;

		//fmt::println("[INTERPRETER] ADDING BLOCK AT {:#010x}", address);

		if (!IsCacheable(address)) [[unlikely]]
			return;

		auto region			= MEMORY_RANGE(address >> 24);
		auto region_offset  = (address & REGIONS_LEN[u8(region)]) >> 1;

		auto region_ptr = GetBlockRegion(address);

		auto& block_ptr = (*region_ptr)[region_offset];
		if (block_ptr) [[unlikely]] {
			//This might be a problem, not sure what to do here
			fmt::print("[INTERPRETER] Replacing block at {:#010x}\n", address);
			block_ptr.reset();
		}

		new_block.absolute_address = address;

		//Allocate space for the block and simply 
		//move the contents of the previous block
		//to avoid further allocations
		block_ptr = std::make_unique<Block>(std::move(new_block));

		if (region == MEMORY_RANGE::IWRAM) {
			//Block is inside IWRAM, we need 
			//to keep a secondary reference
			//to the block in case we want
			//to perform invalidation

			//First get the page number
			auto page = GetPageFromAddress(address);
			//Then get the page itself
			auto& block_list = m_iwram_page_blocks[page];

			//Add a reference to the new block
			block_list.blocks.push_back(&block_ptr);
		}
	}

	void InterpreterCache::Invalidate(u32 address, u32 write_size) {
		using namespace memory;

		auto region         = MEMORY_RANGE(address >> 24);

		if (region != MEMORY_RANGE::IWRAM)
			return;

		//Check if we are above a fixed stack value, if we are,
		//this is probably a push/pop, which means we ignore it

		static constexpr u32 STACK_START = IWRAM_END_ADDRESS - IWRAM_STACK_SIZE;

		if (address >= STACK_START)
			return;

		auto end_address = address + write_size - 1;

		auto first_page  = GetPageFromAddress(address);
		auto second_page = GetPageFromAddress(end_address);

		if (first_page != second_page) [[unlikely]] {
			fmt::print("[INTERPRETER] Write crosses page boundary!\n");
			error::DebugBreak();
		}

		if (m_curr_block) [[likely]] {
			auto curr_block_page   = GetPageFromAddress(m_curr_block->absolute_address);
			auto curr_block_region = MEMORY_RANGE(m_curr_block->absolute_address >> 24);

			if (region == curr_block_region &&
				first_page == curr_block_page) {
				fmt::print("[INTERPRETER] Invalidating current block!\n");
				m_curr_block = nullptr;
			}
		}

		//Get page
		auto& block_list = m_iwram_page_blocks[first_page];

		if (!block_list.blocks.empty()) {
			//For each block in the region,
			//invalidate
			for (auto block_ptr : block_list.blocks) {
				block_ptr->reset();
			}

			//fmt::println("[INTERPRETER] Invalidated {} blocks", block_list.blocks.size());
			//All blocks are invalid, 
			//clear the list of blocks
			block_list.blocks.clear();
		}
	}

	u32 InterpreterCache::GetPageFromAddress(u32 address) const {
		using namespace memory;

		auto region = MEMORY_RANGE(address >> 24);
		auto region_offset = (address & REGIONS_LEN[u8(region)]);
		auto page_in_region = region_offset >> m_page_shift;

		return page_in_region;
	}
}