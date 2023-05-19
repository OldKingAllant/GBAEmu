#pragma once

union SDL_Event;

#include "../common/Defs.hpp"

namespace GBA::input {
	class Keypad;
}

namespace GBA::video {
	class VideoOutput {
	public :
		VideoOutput() :
			m_scale_x{1},
			m_scale_y{1},
			m_stop{false},
			m_keypad(nullptr)
		{}

		static constexpr unsigned GBA_HEIGHT = 160;
		static constexpr unsigned GBA_WIDTH = 240;

		virtual bool Init(unsigned scale_x, unsigned scale_y) = 0;

		virtual bool ConfirmEventTarget(SDL_Event* ev) = 0;
		virtual void ProcessEvent(SDL_Event* ev) = 0;

		virtual void PresentFrame() = 0;
		virtual void SetFrame(float* buffer) = 0;

		void SetKeypad(input::Keypad* keypad) {
			m_keypad = keypad;
		}

		bool Stopped() const {
			return m_stop;
		}

		virtual ~VideoOutput() {};

	protected :
		unsigned m_scale_x;
		unsigned m_scale_y;

		bool m_stop;

		input::Keypad* m_keypad;
	};
}