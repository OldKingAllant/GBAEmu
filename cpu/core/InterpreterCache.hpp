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

	/// <summary>
	/// A single instruction
	/// inside a block
	/// </summary>
	struct BlockEntry {
		u32 orig_instruction{};
		void* thumb_func{nullptr};
		void*   arm_func{nullptr};
	};

	enum class WaitloopState {
		NOT_EVALUATED = 0,
		NOT_WAITLOOP  = 1,
		WAITLOOP      = 2
	};

	/// <summary>
	/// A block of instructions
	/// </summary>
	struct Block {
		//The instruction set (ARM/THUMB) used when
		//the block has been first created
		InstructionMode instr_set = InstructionMode::ARM;
		//Self-explanatory
		u32 absolute_address = {};
		std::vector<BlockEntry> instructions = {};
		WaitloopState waitloop_evaluation = {};
		u32 poll_address = {};

		Block(Block&& other) noexcept :
			instr_set{ other.instr_set },
			absolute_address{ other.absolute_address },
			instructions{std::move(other.instructions)},
			waitloop_evaluation{other.waitloop_evaluation},
			poll_address{other.poll_address} {}

		Block() = default;
	};

	/// <summary>
	/// This is used for IWRAM invalidation,
	/// for each page we have a list of
	/// pointers to the blocks inside that page,
	/// so that we may invalidate all of them
	/// in one go
	/// </summary>
	struct BlockList {
		std::list<std::unique_ptr<Block>*> blocks;
	};

	class InterpreterCache {
	public :
		InterpreterCache();

		void SetBlocksLen(u32 block_len);
		void SetPageLen(u32 region_sz);

		inline u32 GetBlockLen() const {
			return m_block_len;
		}

		inline u32 GetPageLen() const {
			return m_page_len;
		}

		/// <summary>
		/// Inside IWRAM, get the page from
		/// the given address
		/// </summary>
		/// <param name="address"></param>
		/// <returns>The page number</returns>
		u32 GetPageFromAddress(u32 address) const;

		void Init();

		/// <summary>
		/// Get the block starting at the given
		/// address (if any).
		/// </summary>
		/// <param name="address"></param>
		/// <returns>nullptr -> no block cannot cache, *block = nullptr -> no block</returns>
		Block** GetBlock(u32 address);

		/// <summary>
		/// Add block at the provided address
		/// </summary>
		/// <param name="address">Where to place the block</param>
		/// <param name="new_block">The new block (invalidated after function call)</param>
		void AddBlock(u32 address, Block&& new_block);

		/// <summary>
		/// Invalidate range of blocks starting from
		/// the given address
		/// </summary>
		/// <param name="address"></param>
		/// <param name="write_size"></param>
		void Invalidate(u32 address, u32 write_size);

		bool IsCacheable(u32 address) const;

		static constexpr u32 IWRAM_END_ADDRESS = (4 << 24);
		static constexpr u32 IWRAM_STACK_SIZE  = 0x400;

	private :
		/// <summary>
		/// Get pointer to the entire region in which the address
		/// is found
		/// </summary>
		/// <param name="address"></param>
		/// <returns>Pointer to region</returns>
		std::vector<std::unique_ptr<Block>>* GetBlockRegion(u32 address);

	private :
		//Max block len (in bytes)
		u32 m_block_len;
		
		//IWRAM Page size. Blocks cannot
		//cross page boundaries
		u32 m_page_len;
		u32 m_page_shift;

		//The various caches for cacheable regions

		std::vector<std::unique_ptr<Block>> m_bios_cache;
		std::vector<std::unique_ptr<Block>> m_rom_cache;
		std::vector<std::unique_ptr<Block>> m_iwram_cache;

		//Each entry represents an IWRAM page,
		//with a list of pointers to blocks
		//which live inside said page
		std::vector<BlockList> m_iwram_page_blocks;

		//Current block pointer, used for invalidation

		Block* m_curr_block;
	};
}