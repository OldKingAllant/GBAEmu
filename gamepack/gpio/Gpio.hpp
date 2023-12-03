#pragma once

#include "../../common/Defs.hpp"
#include "../../gamepack/backups/Database.hpp"

#include <vector>

namespace GBA::gamepack::gpio {
	using namespace common;

	class GpioDeviceBase;

	enum class PortDirection : u8 {
		IN,
		OUT
	};

	class Gpio {
	public:
		Gpio();

		void AddDevice(backups::GpioDevices dev, backups::PinConnections const& connections);

		void WritePorts(u8 value);
		u8 ReadPorts() const;

		void WriteDirs(u8 value);
		u8 ReadDirs() const;

		void WriteControl(u8 value);
		u8 ReadControl() const;

		~Gpio();
	private :
		std::vector<std::pair<GpioDeviceBase*, backups::PinConnections>> m_devs;

		union {
			struct {
				PortDirection port_0_dir : 1;
				PortDirection port_1_dir : 1;
				PortDirection port_2_dir : 1;
				PortDirection port_3_dir : 1;
			};

			u8 raw;
		} m_port_directions;

		bool m_read_write;

		static_assert(sizeof(m_port_directions) == 1);
	};
}