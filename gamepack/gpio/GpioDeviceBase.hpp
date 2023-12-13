#pragma once

#include "../../common/Defs.hpp"

namespace GBA::gamepack::gpio {
	class GpioDeviceBase {
	public :
		GpioDeviceBase() = default;

		virtual common::u8 Read() = 0;
		virtual void Write(common::u8 pin_values) = 0;

		virtual ~GpioDeviceBase() = default;
	};
}