#pragma once

#include "./Backup.hpp"

namespace GBA::gamepack::backups {
	using namespace common;

	enum class EEPROM_Status {
		IDLE = 1,
		WAITING_ADDRESS = (1 << 1),
		WAITING_DUMMY = (1 << 2),
		READING = (1 << 3),
		WRITING = (1 << 4)
	};

	static u8 operator&(EEPROM_Status left, EEPROM_Status right) {
		return (u8)left & (u8)right;
	}

	static EEPROM_Status operator|(EEPROM_Status left, EEPROM_Status right) {
		return (EEPROM_Status)( (u8)left | (u8)right );
	}

	static EEPROM_Status operator^(EEPROM_Status left, EEPROM_Status right) {
		return (EEPROM_Status)((u8)left ^ (u8)right);
	}

	class EEPROM : public Backup {
	public :
		EEPROM(std::streamsize rom_sz, u32 sz);

		bool Load(std::filesystem::path const& from) override;
		bool Store(std::filesystem::path const& to) override;

		u32 Read(u32 address) override;
		void Write(u32 address, u32 value) override;

		~EEPROM() override;

		static constexpr u32 BLOCK_SIZE = 0x8;

	private :
		u8* m_data;
		u32 m_address_mask;

		EEPROM_Status m_status;

		std::uint64_t m_read_buffer;
		u8 m_pos;
		u8 m_expected_bits;
		u8 m_recvd_bits;
		std::uint64_t m_serial_buffer;
		u16 m_internal_address;

		u8 m_waiting_address_num_bits;
	};
}