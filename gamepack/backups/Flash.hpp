#pragma once

#include "../../gamepack/backups/Backup.hpp"

namespace GBA::gamepack::backups {
	enum class FlashStatus {
		WAITING_PRE_COMMAND0,
		WAITING_PRE_COMMAND1,
		WAITING_COMMAND
	};

	enum class FlashMode {
		NORMAL,
		ID_MODE,
		BANK_SWITCH,
		ERASE,
		WRITE_BYTE
	};

	// 09C2h  Macronix   128K

	class Flash : public Backup {
	public :
		Flash(bool sz_type, std::streamoff rom_sz);

		bool Load(std::filesystem::path const& from) override;
		bool Store(std::filesystem::path const& to) override;

		u32 Read(u32 address) override;
		void Write(u32 address, u32 value) override;

		~Flash() override;

		constexpr static common::u8 DEVICE = 0x09;
		constexpr static common::u8 MANUFACTURER = 0xC2;

	private :
		common::u8* m_data;
		FlashStatus m_status;
		FlashMode m_mode;

		u32 m_curr_bank;
		uint64_t m_total_banks;

		void NormalMode_ProcessPacket(common::u32 address, common::u8 data);
		void IdMode_ProcessPacket(common::u32 address, common::u8 data);
		void BankSwitch_ProcessPacket(common::u32 address, common::u8 data);
		void Erase_ProcessPacket(common::u32 address, common::u8 data);
		__declspec(noinline) void Write_ProcessPacket(common::u32 address, common::u8 data);
	};
}