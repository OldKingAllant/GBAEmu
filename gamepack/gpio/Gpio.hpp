#pragma once

#include "../../common/Defs.hpp"
#include "../../gamepack/backups/Database.hpp"

#include "GpioDeviceBase.hpp"
#include "RTC.hpp"

#include <vector>

namespace GBA::gamepack::gpio {
	using namespace common;

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

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_port_directions.raw);
			ar(m_read_write);

			for (auto const& [dev, _] : m_devs) {
				switch (dev->GetDevType())
				{
				case GpioDevType::RTC:
					ar(*dynamic_cast<RTC const*>(dev));
					break;
				default:
					break;
				}
			}
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_port_directions.raw);
			ar(m_read_write);

			for (auto& [dev, _] : m_devs) {
				switch (dev->GetDevType())
				{
				case GpioDevType::RTC:
					ar(*dynamic_cast<RTC*>(dev));
					break;
				default:
					break;
				}
			}
		}

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