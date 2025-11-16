#include "TASParser.hpp"

#include "../thirdparty/nlohmann_json/json.hpp"

#include <fstream>
#include <memory>

#include <fmt/format.h>

namespace GBA::tas {
	std::optional<std::vector<Action>> _ParseFile(std::filesystem::path const& path) {
		if (!std::filesystem::exists(path)) {
			fmt::println("[TAS] File {} does not exist", path.string());
			return std::nullopt;
		}

		std::ifstream tas_file{ path, std::ios::in | std::ios::beg };
		if (!tas_file.is_open()) {
			fmt::println("[TAS] Could not open {}", path.string());
			return std::nullopt;
		}

		nlohmann::json directives = nlohmann::json::parse(tas_file);
		if (!directives.is_array()) {
			fmt::println("[TAS] File {} has invalid format", path.string());
			return std::nullopt;
		}

		std::vector<Action> action_list{};

		size_t tot_frames = {};

		for (size_t line = 0;  auto const& directive : directives) {
			line++;
			if (!directive.is_array() || !directive.rbegin()->is_number()) {
				fmt::println("[TAS] Directive at line {} is invalid", line);
				continue;
			}

			Action curr_action{};
			curr_action.buttons = 0x3FF;

			bool is_valid = true;

			auto end = (directive.rbegin() + 1).base();
			for (auto curr = directive.begin(); curr != end; curr++) {
				if (!curr->is_string()) {
					fmt::println("[TAS] Directive at line {} is invalid", line);
					is_valid = false;
					break;
				}

				auto as_string = curr->get<std::string>();
				auto pos = g_ACTION_BUTTON_MAP.find(as_string);
				if (pos == g_ACTION_BUTTON_MAP.end()) {
					fmt::println("[TAS] Directive at line {} has invalid action, ignoring", line);
					continue;
				}

				curr_action.buttons &= ~(pos->second) & 0x3FF;
			}

			if (is_valid) {
				auto frame_count = directive.rbegin()->get<int>();
				if (frame_count < 1) {
					fmt::println("[TAS] Directive at line {} has frame count <= 0", line);
				}
				curr_action.num_frames = u32(std::clamp(frame_count, 0, frame_count));
				tot_frames += curr_action.num_frames;
				action_list.push_back(curr_action);
			}
		}

		double s = double(tot_frames) / 60.0;
		double m = s / 60.0;
		double h = m / 60.0;
		double days = int64_t(h / 24.0);

		s = double(int64_t(s) % 60);
		m = double(int64_t(m) % 60);
		h = double(int64_t(h) % 24);

		fmt::println("[TAS] Loaded TAS file:");
		fmt::println("      {} actions", action_list.size());
		fmt::println("      {} frames", tot_frames);

		if (days > 0) {
			fmt::println("      for {} days and {}:{}:{} emulated time", days, h, m, s);
		}
		else {
			fmt::println("      for {}:{}:{} emulated time", h, m, s);
		}

		return action_list;
	}
}