#pragma once

#include "CPUContext.hpp"
#include "InterpreterCache.hpp"

namespace GBA::memory {
	class InterruptController;
	class EventScheduler;
}

namespace GBA::cpu {
	class ARM7TDI {
	public :
		ARM7TDI();

		void AttachBus(memory::Bus* bus);

		void SkipBios();
		void Step();
		
		static u32 GetExceptVector(ExceptionCode const& exc);
		static Mode GetModeFromExcept(ExceptionCode const& exc);

		CPUContext& GetContext() {
			return m_ctx;
		}

		CPUContext const& GetContext() const {
			return m_ctx;
		}

		void SetInterruptControl(memory::InterruptController* int_controller);

		inline void SetHalted() {
			m_halt = true;
		}

		inline void ResetHalted() {
			m_halt = false;
		}

		inline bool IsHalted() const {
			return m_halt;
		}

		void EvaluateHaltState();

		void SetInterpreterBlockSize(u32 block_size) {
			m_cache.SetBlocksLen(block_size);
		}

		void SetInterpreterPageSize(u32 region_sz) {
			m_cache.SetPageLen(region_sz);
		}

		void EnableCachedInterpreter() {
			m_cache.Init();
			m_enable_cache = true;
		}

		void EnableWaitloopDetection() {
			m_enable_waitloop_detection = true;
		}

		inline bool IsCacheEnabled() const {
			return m_enable_cache;
		}

		InterpreterCache& GetCache() {
			return m_cache;
		}

		inline bool IsInBlock() const {
			return m_in_block;
		}

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_halt);
			ar(m_ctx.m_regs);
			ar(m_ctx.m_cpsr);
			ar(m_ctx.m_spsr);
			ar(m_ctx.m_pipeline);
			ar(m_ctx.m_old_pc);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_halt);
			ar(m_ctx.m_regs);
			ar(m_ctx.m_cpsr);
			ar(m_ctx.m_spsr);
			ar(m_ctx.m_pipeline);
			ar(m_ctx.m_old_pc);
		}

	private :
		bool CheckIRQ();
		bool CheckIRQ_NoReset();

		void				 StepCached();
		std::pair<bool, u32> StepNoCache();
		void				 RunMakeCache();

		void                EvaluateLoop(Block& loop);
		[[nodiscard]] bool   RunWaitLoop(Block& loop);

	private :
		CPUContext				m_ctx;
		memory::Bus*			m_bus;
		memory::InterruptController* m_int_controller;
		bool					m_halt;
		InterpreterCache		m_cache;
		bool					m_enable_cache;
		bool					m_enable_waitloop_detection;
		bool					m_in_block;
		memory::EventScheduler* m_sched;
	};
}