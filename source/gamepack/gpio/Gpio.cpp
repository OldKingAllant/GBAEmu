#include "../../../gamepack/gpio/Gpio.hpp"

#include "../../../gamepack/gpio/RTC.hpp"

namespace GBA::gamepack::gpio {
	Gpio::Gpio() : m_devs{}, m_port_directions{},
		m_read_write{false}
	{}

	void Gpio::AddDevice(backups::GpioDevices dev, backups::PinConnections const& connections) {
		GpioDeviceBase* new_dev = nullptr;

		switch (dev)
		{
		case backups::GpioDevices::RTC:
			new_dev = new RTC();
			break;

		default:
			break;
		}

		if(new_dev)
			m_devs.push_back({ new_dev, connections });
	}

	void Gpio::WritePorts(u8 value) {
		u8 new_val = 0;
		//value &= m_port_directions.raw;

		for (auto& [dev, conns] : m_devs) {
			for (int pin = 0; pin < 4; pin++) {
				if (conns.conns[pin] != -1) {
					u8 bit = (value >> conns.conns[pin]) & 1;
					new_val |= (bit << pin);
				}
			}

			dev->Write(new_val);
		}
	}

	u8 Gpio::ReadPorts() const {
		if(!m_read_write)
			return 0x0;

		u8 ret = 0;

		for (auto& [dev, conns] : m_devs) {
			u8 read = dev->Read() & 0xF;
			u8 res = 0;

			for (int pin = 0; pin < 4; pin++) {
				if (conns.conns[pin] != -1) {
					u8 bit = (read >> pin) & 1;
					res |= (bit << conns.conns[pin]);
				}
			}

			ret |= res;
			ret &= ~m_port_directions.raw;
		}

		return ret;
	}

	void Gpio::WriteDirs(u8 value) {
		value &= 0x0F;
		m_port_directions.raw = value;
	}

	u8 Gpio::ReadDirs() const {
		if (!m_read_write)
			return 0x0;

		return m_port_directions.raw;
	}

	void Gpio::WriteControl(u8 value) {
		m_read_write = value & 1;
	}

	u8 Gpio::ReadControl() const {
		if (!m_read_write)
			return 0x0;

		return m_read_write;
	}

	Gpio::~Gpio() {
		for (auto& [dev, _] : m_devs) {
			delete dev;
		}
	}
}