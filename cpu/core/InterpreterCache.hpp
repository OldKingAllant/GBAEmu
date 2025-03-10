#pragma once

#include "../../common/Defs.hpp"
#include "../../common/FixedSizeAllocator.hpp"

#include "../ProcessorDefs.hpp"
#include "CPUContext.hpp"

#include <list>
#include <memory>
#include <vector>

namespace GBA::memory {
	class Bus;
}

namespace GBA::cpu {
	using namespace common;

	struct BlockEntry {
		u32 orig_instruction{};
		void* thumb_func{nullptr};
		void*   arm_func{nullptr};
	};

	struct Block {
		InstructionMode instr_set = InstructionMode::ARM;
		u32 base_address = {};
		u32 region       = {};
		u32 mem_range    = {};
		std::vector<BlockEntry> instructions = {};

		Block(Block&& other) noexcept :
			instr_set{ other.instr_set },
			base_address{ other.base_address },
			region{ other.region },
			mem_range{ other.mem_range },
			instructions{std::move(other.instructions)} { }

		Block() = default;
	};

	struct BlockList {
		std::list<Block> blocks;
	};

	class InterpreterCache {
	public :
		InterpreterCache();

		void SetBlocksLen(u32 block_len);
		void SetRegionLen(u32 region_sz);

		inline u32 GetBlockLen() const {
			return m_block_len;
		}

		inline u32 GetRegionLen() const {
			return m_region_len;
		}

		u32 GetRegionFromAddress(u32 address) const;

		void Init();

		__declspec(noinline) Block** GetBlock(u32 address);
		void AddBlock(u32 address, Block&& new_block);
		void Invalidate(u32 address, u32 write_size);

		bool IsCacheable(u32 address) const;

	private :
		BlockList* GetBlockList(u32 region, u32 page) const;

	private :
		u32 m_block_len;
		u32 m_region_len;
		u32 m_region_shift;

		std::unique_ptr<BlockList[]> m_bios_cache;
		std::unique_ptr<BlockList[]> m_rom_cache;
		std::unique_ptr<BlockList[]> m_iwram_cache;

		Block* m_curr_block;
	};
}