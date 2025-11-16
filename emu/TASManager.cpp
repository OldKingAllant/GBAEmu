#include "TASManager.hpp"
#include "Emulator.hpp"

#include "../memory/Keypad.hpp"

#include <fmt/format.h>
#include <fstream>

#include "../thirdparty/nlohmann_json/json.hpp"

namespace GBA::tas {
	TASManager::TASManager(emulation::Emulator* emu) :
		m_actions{}, m_curr_action{}, 
		m_curr_action_frames{}, m_emu{emu}, 
		m_callback_id{}, m_curr_frame{},
		m_last_frame{} {}

	void TASManager::SetActions(std::vector<Action>&& acts) {
		m_actions = std::move(acts);
		m_curr_action = 0;
		m_curr_frame = 0;
		m_last_frame = 0;
		m_curr_action_frames = 0;
		if (m_emu->IsBootComplete()) {
			BootComplete();
		}
	}

	void TASManager::BootComplete() {
		if (!m_actions.empty()) {
			Action first_action = m_actions[0];
			m_emu->GetContext().keypad.KeyPressed(input::Buttons(~first_action.buttons & 0x3FF));
		}
	}

	void TASManager::SetupCallback() {
		constexpr u32 KEYSTAT_ADDRESS = (u32(memory::MEMORY_RANGE::IO) << 24) +
			input::Keypad::KEYPAD_REG_OFFSET;
		m_callback_id = m_emu->AddWatchpoint(KEYSTAT_ADDRESS, false, [this](uint32_t& val_dest) {
			if (m_curr_action < m_actions.size()) {
				if (m_last_frame == m_curr_frame) {
					fmt::println("[TAS] Read happened multiple times in a frame");
				}
				m_last_frame = m_curr_frame;
				auto const& action = m_actions[m_curr_action];
				m_curr_action_frames++;
				if (action.num_frames < m_curr_action_frames) {
					//fmt::println("[TAS] New action, old: {:05x}, new id: {}", action.buttons, m_curr_action + 1);
					++m_curr_action;
					m_curr_action_frames = 0;
					m_emu->GetContext().keypad.KeyReleased(input::Buttons(~action.buttons & 0x3FF));
					if (m_emu->GetContext().keypad.GetKeyStatus() != 0x3FF) {
						fmt::println("[TAS] Invalid key state!");
					}
				}
				else {
					m_emu->GetContext().keypad.KeyPressed(input::Buttons(~action.buttons & 0x3FF));
				}
			}
			return false;
		}).value();
	}

	void TASManager::RemoveCallback() {
		m_emu->RemoveWatchpoint(m_callback_id);
	}

	void TASManager::FrameDone() {
		m_curr_frame++;

		if (m_emu->IsBootComplete()) {
			if (m_curr_action < m_actions.size()) {
				auto const& action = m_actions[m_curr_action];
				m_curr_action_frames++;
				if (action.num_frames < m_curr_action_frames) {
					fmt::println("[TAS] New action, old: {}, new id: {}", action.buttons, m_curr_action+1);
					++m_curr_action;
					m_curr_action_frames = 0;
					m_emu->GetContext().keypad.KeyReleased(input::Buttons(~action.buttons & 0x3FF));
				}
				else {
					m_emu->GetContext().keypad.KeyPressed(input::Buttons(~action.buttons & 0x3FF));
				}
			}
		}
		
		if (m_curr_action == m_actions.size() && !m_actions.empty()) {
			EndOfFile();
		}
	}

	bool TASManager::DumpRemainingActions(std::string const& path) const {
		if (!HasActions()) {
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

		auto start = m_actions.cbegin() + m_curr_action;
		for (auto curr_action = start; curr_action < m_actions.cend(); curr_action++) {
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

			uint32_t rem_frames = curr_action->num_frames;

			if (curr_action == start) {
				rem_frames -= m_curr_action_frames;
			}

			curr_entry.push_back(rem_frames);
			json_dump.emplace_back(std::move(curr_entry));
		}

		out_file << std::setw(1) << json_dump;

		fmt::println("[TAS] Stored {} actions", json_dump.size());

		return true;
	}

	void TASManager::EndOfFile() {
		//RemoveCallback();
	}
}