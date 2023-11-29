#include "../../../gamepack/backups/Flash.hpp"

#include "../../../common/Defs.hpp"
#include "../../../common/Error.hpp"

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
		return false;
	}

	bool Flash::Store(std::filesystem::path const& to) {
		return false;
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

		return m_data[address & 0xFFFF];
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
		default:
			break;
		}
	}

	Flash::~Flash() {
		if (m_data)
			delete[] m_data;
	}
}