#include "../../../gamepack/backups/Flash.hpp"

#include "../../../common/Defs.hpp"
#include "../../../common/Error.hpp"

#include "../../../common/Logger.hpp"

#include <fstream>

namespace GBA::gamepack::backups {
	using namespace common;

	Flash::Flash(bool sz_type, std::streamoff rom_sz) : Backup(rom_sz), m_data(nullptr),
	m_status(FlashStatus::WAITING_PRE_COMMAND0), 
		m_mode(FlashMode::NORMAL), m_curr_bank{}, m_total_banks{1} {
		m_type = BackupType::FLASH;
		m_bus_mask = 0xFF;
		m_start_address = 0xE000000;

		if (sz_type) {
			m_data = new u8[128 * 1024];
			m_total_banks = 2;
		}
		else
			m_data = new u8[64 * 1024];

		std::fill_n(m_data, 64 * 1024 * m_total_banks, 0xFF);
	}

	bool Flash::Load(std::filesystem::path const& from) {
		std::streamsize req_size = m_total_banks * 64 * 1024;

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

	bool Flash::Store(std::filesystem::path const& to) {
		std::ofstream save_file(to, std::ios::out);

		if (!save_file.is_open())
			return false;

		save_file.close();

		save_file.open(to, std::ios::out | std::ios::binary);

		save_file.write(reinterpret_cast<char*>(m_data), m_total_banks * 64 * 1024);

		return true;
	}

	u32 Flash::Read(u32 address) {
		if (m_mode == FlashMode::ID_MODE) {
			if (address == 0x0) {
				return MANUFACTURER;
			}
			else if (address == 0x1) {
				return DEVICE;
			}
		}

		//logging::Logger::Instance().LogInfo("FLASH", " Reading address 0x{:x}  -> 0x{:x}",
			//address, m_data[address & 0xFFFF]);

		return m_data[(address & 0xFFFF) + (m_curr_bank * 64 * 1024)];
	}

	void Flash::NormalMode_ProcessPacket(u32 address, u8 data) {
		switch (m_status)
		{
		case GBA::gamepack::backups::FlashStatus::WAITING_PRE_COMMAND0: {
			if (address == 0x5555 && data == 0xAA)
				m_status = FlashStatus::WAITING_PRE_COMMAND1;
		}
		break;
		case GBA::gamepack::backups::FlashStatus::WAITING_PRE_COMMAND1: {
			if (address == 0x2AAA && data == 0x55)
				m_status = FlashStatus::WAITING_COMMAND;
		}
		break;
		case GBA::gamepack::backups::FlashStatus::WAITING_COMMAND: {
			if (address == 0x5555) {
				if (data == 0x90) {
					m_status = FlashStatus::WAITING_PRE_COMMAND0;
					m_mode = FlashMode::ID_MODE;
				}
				else if (data == 0xB0) {
					m_status = FlashStatus::WAITING_PRE_COMMAND0;
					m_mode = FlashMode::BANK_SWITCH;
				}
				else if (data == 0x80) {
					m_status = FlashStatus::WAITING_PRE_COMMAND0;
					m_mode = FlashMode::ERASE;
				}
				else if (data == 0xA0) {
					m_status = FlashStatus::WAITING_PRE_COMMAND0;
					m_mode = FlashMode::WRITE_BYTE;
				}
			}
			else
				error::DebugBreak();
		}
		break;
		default:
			error::Unreachable();
			break;
		}
	}

	void Flash::IdMode_ProcessPacket(u32 address, u8 data) {
		switch (m_status)
		{
		case GBA::gamepack::backups::FlashStatus::WAITING_PRE_COMMAND0: {
			if (address == 0x5555 && data == 0xAA)
				m_status = FlashStatus::WAITING_PRE_COMMAND1;
		}
		break;
		case GBA::gamepack::backups::FlashStatus::WAITING_PRE_COMMAND1: {
			if (address == 0x2AAA && data == 0x55)
				m_status = FlashStatus::WAITING_COMMAND;
		}
		break;
		case GBA::gamepack::backups::FlashStatus::WAITING_COMMAND: {
			if (address == 0x5555) {
				if (data == 0xF0) {
					m_status = FlashStatus::WAITING_PRE_COMMAND0;
					m_mode = FlashMode::NORMAL;
				}
				else
					error::DebugBreak();
			}
			else
				error::DebugBreak();
		}
		break;
		default:
			error::Unreachable();
			break;
		}
	}

	void Flash::BankSwitch_ProcessPacket(u32 address, u8 data) {
		if (address == 0x0) {
			if (data >= m_total_banks)
				error::DebugBreak();
			m_curr_bank = data;
			m_mode = FlashMode::NORMAL;
		}
		else if (address == 0x5555 && data == 0xF0)
			m_mode = FlashMode::NORMAL;
		else
			error::DebugBreak();
	}

	void Flash::Erase_ProcessPacket(u32 address, u8 data) {
		switch (m_status)
		{
		case GBA::gamepack::backups::FlashStatus::WAITING_PRE_COMMAND0: {
			if (address == 0x5555 && data == 0xAA)
				m_status = FlashStatus::WAITING_PRE_COMMAND1;
		}
		break;
		case GBA::gamepack::backups::FlashStatus::WAITING_PRE_COMMAND1: {
			if (address == 0x2AAA && data == 0x55)
				m_status = FlashStatus::WAITING_COMMAND;
		}
		break;
		case GBA::gamepack::backups::FlashStatus::WAITING_COMMAND: {
			if (address == 0x5555) {
				if (data == 0xF0) {
					m_status = FlashStatus::WAITING_PRE_COMMAND0;
					m_mode = FlashMode::NORMAL;
				}
				else if (data == 0x10) {
					m_status = FlashStatus::WAITING_PRE_COMMAND0;
					m_mode = FlashMode::NORMAL;
					//Erase entire chip
					std::fill_n(m_data, 64 * 1024 * m_total_banks, 0xFF);
				}
				else
					error::DebugBreak();
			}
			else {
				if ((address & 0xFFF) == 0 && data == 0x30) {
					u32 sector_n = address & 0xF000;

					std::fill_n(m_data + (m_curr_bank * 64 * 1024) + sector_n, 0x1000, 0xFF);
				}

				m_status = FlashStatus::WAITING_PRE_COMMAND0;
				m_mode = FlashMode::NORMAL;
			}
		}
		break;
		default:
			error::Unreachable();
			break;
		}
	}

	void Flash::Write_ProcessPacket(u32 address, u8 data) {
		if (address != 0x5555 || data != 0xF0) {
			m_data[(m_curr_bank * 64 * 1024) + address] = data;
		}

		m_mode = FlashMode::NORMAL;
		m_status = FlashStatus::WAITING_PRE_COMMAND0;
	}

	void Flash::Write(u32 address, u32 value) {
		address &= 0xFFFF;
		value &= 0xFF;

		switch (m_mode)
		{
		case GBA::gamepack::backups::FlashMode::NORMAL:
			NormalMode_ProcessPacket(address, value);
			break;
		case GBA::gamepack::backups::FlashMode::ID_MODE:
			IdMode_ProcessPacket(address, value);
			break;
		case GBA::gamepack::backups::FlashMode::BANK_SWITCH:
			BankSwitch_ProcessPacket(address, value);
			break;
		case GBA::gamepack::backups::FlashMode::ERASE:
			Erase_ProcessPacket(address, value);
			break;
		case GBA::gamepack::backups::FlashMode::WRITE_BYTE:
			Write_ProcessPacket(address, value);
			break;
		default:
			break;
		}
	}

	Flash::~Flash() {
		if (m_data)
			delete[] m_data;
	}
}