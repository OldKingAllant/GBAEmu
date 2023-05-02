#include "Debugger.hpp"

#include "../emu/Emulator.hpp"

#include <algorithm>

#include "../cpu/core/Register.hpp"
#include "../common/Logger.hpp"

namespace GBA::debugger {
	LOG_CONTEXT(Debugger);

	Debugger::Debugger(emulation::Emulator& emu) :
		m_emu{emu} {}

	emulation::Emulator& Debugger::GetEmulator() {
		return m_emu;
	}

	void Debugger::AddBreakpoint(unsigned breakpoint) {
		if (std::find_if(
			m_breakpoints.begin(), m_breakpoints.end(),
			[breakpoint](Breakpoint const& br) {
				return br.address == breakpoint;
			}
		) != m_breakpoints.end()) {
			return;
		}

		m_breakpoints.push_back(Breakpoint{ breakpoint, true });
	}

	void Debugger::RemoveBreakpoint(unsigned breakpoint) {
		auto pos = std::find_if(
			m_breakpoints.begin(), m_breakpoints.end(),
			[breakpoint](Breakpoint const& br) {
				return br.address == breakpoint;
			}
		);

		if (pos == m_breakpoints.end()) {
			return;
		}

		m_breakpoints.erase(pos);
	}

	void Debugger::ToggleBreakpoint(unsigned breakpoint) {
		auto pos = std::find_if(
			m_breakpoints.begin(), m_breakpoints.end(),
			[breakpoint](Breakpoint const& br) {
				return br.address == breakpoint;
			}
		);

		if (pos == m_breakpoints.end()) {
			return;
		}

		pos->enabled = !pos->enabled;
	}

	std::vector<Breakpoint>& Debugger::GetBreakpoints() {
		return m_breakpoints;
	}

	void Debugger::RunTillBreakpoint(EmulatorStatus& status) {
		auto breakpoint_hit = [this](unsigned address) {
			return std::find_if(
				m_breakpoints.begin(), m_breakpoints.end(),
				[address](Breakpoint const& br) {
					return br.address == address && br.enabled;
				}
			) != m_breakpoints.end();
		};

		bool did_hit_breakpoint = false;

		auto& ctx = m_emu.GetContext().processor.GetContext();

		while (!did_hit_breakpoint) {
			m_emu.EmulateFor(1);

			did_hit_breakpoint = breakpoint_hit(ctx.m_regs.GetReg(15));

			if (status.update_ui)
				break;
		}

		if (did_hit_breakpoint) {
			status.until_breakpoint = false;
			status.stopped = true;
		}
	}

	void Debugger::Run(EmulatorStatus& status) {
		if (status.stopped)
			return;

		if (status.single_step) {
			m_emu.EmulateFor(1);
			status.stopped = true;
			status.single_step = false;
		}

		if (status.until_breakpoint)
			RunTillBreakpoint(status);
	}
}