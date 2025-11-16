#include "InputRecorder.hpp"

#include "../thirdparty/nlohmann_json/json.hpp"

#include <fstream>
#include <fmt/format.h>


namespace GBA::tas {
	InputRecorder::InputRecorder() :
		m_actions{}
	{}

	void InputRecorder::DoFrame(input::Keypad const& pad) {
		if (m_actions.empty() || m_actions.back().buttons != pad.GetKeyStatus()) {
			m_actions.emplace_back(Action{ .buttons = pad.GetKeyStatus(), .num_frames = 1 });
		}
		else {
			m_actions.back().num_frames++;
		}
	}

	bool InputRecorder::Dump(std::string const& path) const {
		if (m_actions.empty()) {
			return false;
		}

		std::ofstream out_file{ path, std::ios::out };
		if (!out_file.is_open()) {
			return false;
		}

		nlohmann::json json_dump = nlohmann::json::array();

		{
			nlohmann::json init_action = nlohmann::json::array({ "w", 1 });
			json_dump.emplace_back(init_action);
		}

		//auto keystat = m_emu->GetContext().keypad.GetKeyStatus();
		const auto c_BTNCOUNT = uint32_t(std::log2(double(input::Buttons::BUTTON_L)));

		constexpr const char* c_BTN_STR[] = {
				"a", "b", "sel", "st", "r",
				"l", "u", "d", "rt", "lt"
		};

		for (auto curr_action = m_actions.cbegin(); curr_action < m_actions.cend(); curr_action++) {
			nlohmann::json curr_entry = nlohmann::json::array();
			u16 buttons = curr_action->buttons;

			if (buttons == 0x3FF) {
				curr_entry.push_back("w");
			}
			else {
				for (uint32_t btn_id = 0; btn_id <= c_BTNCOUNT; btn_id++) {
					bool is_btn_pressed = (~buttons & u16(1 << btn_id)) != 0;
					if (is_btn_pressed) {
						curr_entry.push_back(c_BTN_STR[btn_id]);
					}
				}
			}

			curr_entry.push_back(curr_action->num_frames);
			json_dump.emplace_back(std::move(curr_entry));
		}

		out_file << std::setw(1) << json_dump;

		fmt::println("[TAS] Stored {} actions", json_dump.size());

		return true;
	}
}