#include "../../../gamepack/backups/EEPROM.hpp"

#include "../../../common/Error.hpp"

#include <fstream>

namespace GBA::gamepack::backups {
	EEPROM::EEPROM(std::streamsize rom_sz, u32 sz)
		: Backup(rom_sz), m_data(nullptr), m_address_mask{},
		m_status(EEPROM_Status::IDLE),
		m_read_buffer{}, m_pos{}, m_expected_bits {},
		m_recvd_bits{}, m_serial_buffer{},
		m_internal_address{}, m_waiting_address_num_bits{} {
		m_type = BackupType::EEPROM;

		m_data = new u8[sz * BLOCK_SIZE];
		m_bus_mask = 0xFFFF;

		m_address_mask = sz == 0x400 ? 0x3FF : 0x3F;

		if (rom_sz <= (long long)16 * 1024 * 1024)
			m_start_address = 0xD000000;
		else
			m_start_address = 0xDFFFF00;

		m_expected_bits = 2;

		m_waiting_address_num_bits = sz == 0x400 ? 14 : 6;
	}

	bool EEPROM::Load(std::filesystem::path const& from) {
		constexpr std::streamsize kb8 = (std::streamsize)8 * 1024;
		constexpr std::streamsize byte512 = 512;

		std::streamsize req_size = m_address_mask == 0x3FF ? kb8 : byte512;

		if (!std::filesystem::exists(from) || !std::filesystem::is_regular_file(from))
			return false;

		if (std::filesystem::file_size(from) != req_size)
			return false;

		std::ifstream save_file(from, std::ios::in | std::ios::binary);

		if (!save_file.is_open())
			return false;

		save_file.read(reinterpret_cast<char*>(m_data), req_size);

		return true;
	}

	bool EEPROM::Store(std::filesystem::path const& to) {
		std::ofstream save_file(to, std::ios::out);

		if (!save_file.is_open())
			return false;

		save_file.close();

		save_file.open(to, std::ios::out | std::ios::binary);

		constexpr std::streamsize kb8 = (std::streamsize)8 * 1024;
		constexpr std::streamsize byte512 = 512;

		save_file.write(reinterpret_cast<char*>(m_data), m_address_mask == 0x3FF ? kb8 : byte512);

		return true;
	}

	/*
	Always assume 16 bit read/writes through the data bus
	*/

	u32 EEPROM::Read(u32 address) {
		switch (m_status)
		{
		case GBA::gamepack::backups::EEPROM_Status::IDLE:
			return 0x1;
		case GBA::gamepack::backups::EEPROM_Status::READING: {
			u16 ret_val = 0;

			m_pos++;

			if (m_pos <= 4) {
				return 0;
			}

			ret_val = (m_read_buffer >> (68 - m_pos)) & 1;
			//m_read_buffer >>= 1;

			if (m_pos == 68) {
				m_status = EEPROM_Status::IDLE;
				m_expected_bits = 2;
				m_pos = 0;
			}

			return ret_val;
		}
			break;
		case GBA::gamepack::backups::EEPROM_Status::WRITING:
			return 0x0;
		default:
			break;
		}

		return 0x0;
	}

	void EEPROM::Write(u32 address, u32 orig_value) {
		//It seems that only the first bit 
		//of the value bus is read by
		//the EEPROM, since only one input
		//line is connected

		u16 value = orig_value & 1;

		if (m_recvd_bits + 1 > 64) [[unlikely]] {
			error::DebugBreak();
		}

		m_serial_buffer = (m_serial_buffer << 1) | (value & 1);

		m_recvd_bits++;

		if (m_recvd_bits < m_expected_bits)
			return;

		m_recvd_bits -= m_expected_bits;

		if (m_status & EEPROM_Status::IDLE) {
			u8 command = m_serial_buffer & 3;

			if (command == 0x2) {
				m_status = EEPROM_Status::WAITING_ADDRESS |
					EEPROM_Status::WRITING |
					EEPROM_Status::WAITING_DUMMY;


				m_expected_bits = m_waiting_address_num_bits;
			}
			else if (command == 0x3) {
				m_status = EEPROM_Status::WAITING_ADDRESS |
					EEPROM_Status::WAITING_DUMMY |
					EEPROM_Status::READING;

				m_expected_bits = m_waiting_address_num_bits;
			}
			else {
				m_recvd_bits = 0;
			}

			m_serial_buffer >>= 2;
		}
		else if (m_status & EEPROM_Status::WAITING_ADDRESS) {
			m_internal_address = (u16)( m_serial_buffer & m_address_mask );

			m_serial_buffer >>= m_waiting_address_num_bits;

			if (m_status & EEPROM_Status::WRITING) {
				m_expected_bits = 64;
			}
			else if (m_status & EEPROM_Status::WAITING_DUMMY) {
				m_expected_bits = 1;
			}

			m_status = m_status ^ EEPROM_Status::WAITING_ADDRESS;
		}
		else if (m_status & EEPROM_Status::WRITING) {
			//All 64 bits received
			*reinterpret_cast<std::uint64_t*>(m_data + m_internal_address * BLOCK_SIZE) =
				m_serial_buffer;

			m_serial_buffer = 0;

			m_expected_bits = 1;
			m_status = m_status ^ EEPROM_Status::WRITING;
		}
		else if (m_status & EEPROM_Status::WAITING_DUMMY) {
			/*if (m_serial_buffer & 1) {
				m_serial_buffer >>= 1;
				m_status = EEPROM_Status::IDLE;
				m_expected_bits = 2;
				return;
			}*/

			m_serial_buffer >>= 1;

			m_status = m_status ^ EEPROM_Status::WAITING_DUMMY;

			if (m_status & EEPROM_Status::READING) {
				m_expected_bits = 0;
				m_read_buffer = *reinterpret_cast<std::uint64_t*>(m_data 
					+ m_internal_address * BLOCK_SIZE);
			}
			else {
				m_status = EEPROM_Status::IDLE;
				m_expected_bits = 2;
			}
		}
		else {
			m_recvd_bits = 0;
			m_serial_buffer = 0;
		}
	}

	EEPROM::~EEPROM() {
		if (m_data)
			delete[] m_data;
	}
}