#include "../../../gamepack/gpio/RTC.hpp"

#include "../../../common/Error.hpp"

#include <chrono>
#include <ctime>

namespace GBA::gamepack::gpio {
	using namespace common;

	RTC::RTC() :
		m_communication_enable{false},
		m_serial_clock{false},
		m_serial_data{false},
		m_in_buffer{}, m_out_buffer{},
		m_status{RtcStatus::WAITING_CODE},
		m_comm_dir{}, m_command{},
		m_recv_bits{}, m_wanted_bits{4},
		m_processed_bits{}, 
		m_stat{},
		m_time_latch{},
		m_date_latch{}
	{}

	//0 -> SCK
	//1 -> SIO
	//2 -> CS
	u8 RTC::Read() {
		if (!m_communication_enable)
			return 0x0;

		return ((u8)(m_serial_clock) | ((u8)m_serial_data << 1) | ((u8)m_communication_enable << 2));
	}

	bool RTC::Command_Stat() {
		if(m_comm_dir == CommandDirection::READ)
			m_serial_data = (bool)(m_stat.raw >> m_processed_bits);
		else {
			u8 mask = (STAT_MASK >> m_processed_bits) & 1;
			u8 bit = (u8)m_serial_data & mask;
			m_stat.raw |= (bit << m_processed_bits);
		}

		m_processed_bits++;

		return m_processed_bits >= 8;
	}

	bool RTC::Command_Reset() {
		if(m_comm_dir == CommandDirection::WRITE)
			m_stat.raw = 0;
		return true;
	}

	void RTC::DateTimeLatch() {
		auto timet = std::chrono::system_clock::to_time_t(
			std::chrono::system_clock::now()
		);

		auto bin_to_bcd = [](u8 value) {
			u8 converted = 0;

			converted |= (value % 10);
			value /= 10;
			converted |= ((value % 10) << 4);

			return converted;
		};

		tm mtm;

		localtime_s(&mtm, &timet);

		m_time_latch.second = bin_to_bcd( (u8)mtm.tm_sec );
		m_time_latch.minute = bin_to_bcd( (u8)mtm.tm_min );
		m_time_latch.hour = (u8)mtm.tm_hour;

		bool am_pm = m_time_latch.hour >= 12;

		if (!m_stat.mode)
			m_time_latch.hour %= 12;

		m_time_latch.hour = bin_to_bcd(m_time_latch.hour);

		if (!m_stat.mode) {
			m_time_latch.hour |= ((u8)am_pm << 6);
		}

		m_date_latch.day = bin_to_bcd( (u8)mtm.tm_mday );
		m_date_latch.day_of_week = bin_to_bcd( (u8)mtm.tm_wday );
		m_date_latch.month = bin_to_bcd( (u8)mtm.tm_mon );
		m_date_latch.year = (u8)mtm.tm_year;

		if (m_date_latch.year > 99)
			m_date_latch.year %= 100;

		m_date_latch.year = bin_to_bcd(m_date_latch.year);
	}

	bool RTC::Command_DayTime() {
		if (m_comm_dir == CommandDirection::WRITE)
			error::DebugBreak();
		else {
			u8 bit = (m_out_buffer >> m_processed_bits) & 1;
			m_serial_data = bit;
		}

		m_processed_bits++;

		return m_processed_bits >= 56;
	}

	bool RTC::Command_Time() {
		if (m_comm_dir == CommandDirection::WRITE)
			error::DebugBreak();
		else {
			u8 bit = (m_out_buffer >> m_processed_bits) & 1;
			m_serial_data = bit;
		}

		m_processed_bits++;

		return m_processed_bits >= 24;
	}

	void RTC::ProcessInput() {
		m_in_buffer = (m_in_buffer << 1) | (uint64_t)m_serial_data;

		m_recv_bits++;

		if (m_recv_bits >= m_wanted_bits)
			m_recv_bits -= m_wanted_bits;
		else
			return;

		switch (m_status)
		{
		case GBA::gamepack::gpio::RtcStatus::WAITING_CODE: {
			if (m_in_buffer == 0b0110) {
				m_status = RtcStatus::WAITING_COMMAND;
				m_wanted_bits = 3;
				m_in_buffer = 0;
			}
			else
				error::DebugBreak();
		}
		break;
		case GBA::gamepack::gpio::RtcStatus::WAITING_COMMAND: {
			if (m_in_buffer > 3)
				error::DebugBreak();

			m_command = (Command)m_in_buffer;
			m_status = RtcStatus::WAITING_DIRECTION;
			m_in_buffer = 0;
			m_wanted_bits = 1;
		}
		break;
		case GBA::gamepack::gpio::RtcStatus::WAITING_DIRECTION: {
			m_comm_dir = m_in_buffer ? CommandDirection::READ : CommandDirection::WRITE;
			m_status = RtcStatus::PROCESSING_COMMAND;
			m_in_buffer = 0;
			m_wanted_bits = 1;

			if (m_comm_dir == CommandDirection::READ) {
				if (m_command == Command::DATETIME ||
					m_command == Command::TIME) {
					DateTimeLatch();

					if (m_command == Command::TIME) {
						m_out_buffer =
							(uint64_t)m_time_latch.hour
							| ((uint64_t)m_time_latch.minute << 8)
							| ((uint64_t)m_time_latch.second << 16);
					}
					else {
						m_out_buffer =
							(uint64_t)m_date_latch.year
							| ((uint64_t)m_date_latch.month << 8)
							| ((uint64_t)m_date_latch.day << 16)
							| ((uint64_t)m_date_latch.day_of_week << 24)
							| ((uint64_t)m_time_latch.hour << 32)
							| ((uint64_t)m_time_latch.minute << 40)
							| ((uint64_t)m_time_latch.second << 48);
					}
				}
			}

			if (m_command == Command::RESET && m_comm_dir == CommandDirection::WRITE) {
				Command_Reset();
				m_status = RtcStatus::WAITING_CODE;
				m_wanted_bits = 4;
				m_processed_bits = 0;
			}
		}
		break;
		case GBA::gamepack::gpio::RtcStatus::PROCESSING_COMMAND: {
			bool cmd_end = false;

			switch (m_command)
			{
			case Command::STAT:
				cmd_end = Command_Stat();
				break;
			case Command::DATETIME:
				cmd_end = Command_DayTime();
				break;
			case Command::TIME:
				cmd_end = Command_Time();
				break;
			default:
				error::Unreachable();
				break;
			}

			m_in_buffer = 0;
			m_wanted_bits = 1;

			if (cmd_end) {
				m_status = RtcStatus::WAITING_CODE;
				m_wanted_bits = 4;
				m_processed_bits = 0;
			}
		}
		break;
		default:
			break;
		}
	}

	//0 -> SCK
	//1 -> SIO
	//2 -> CS
	void RTC::Write(u8 pin_values) {
		//Input data on falling edge of the SCK clock,
		//that data is then processed on the rising
		//edge

		bool old_sck = m_serial_clock;

		m_serial_clock = pin_values & 1;

		m_communication_enable = (pin_values >> 2) & 1;

		//Falling edge
		if(old_sck && !m_serial_clock)
			m_serial_data = (pin_values >> 1) & 1;

		if (!m_communication_enable)
			return;

		if (m_serial_clock && !old_sck) {
			//Rising edge -> process input data
			ProcessInput();
		}
	}
}