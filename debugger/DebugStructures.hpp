#pragma once

#include <optional>

namespace GBA::debugger {
	struct Breakpoint {
		unsigned address;
		bool enabled;
	};

	struct EmulatorStatus {
		bool stopped;
		bool single_step;
		bool until_breakpoint;
		bool update_ui;
	};
}