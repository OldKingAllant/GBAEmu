#pragma once

#include "GpioDeviceBase.hpp"

namespace GBA::gamepack::gpio {
	enum class RtcStatus {
		WAITING_CODE,
		WAITING_COMMAND,
		PROCESSING_COMMAND,
		WAITING_DIRECTION
	};

	enum class CommandDirection {
		WRITE,
		READ
	};

	enum class Command {
		RESET = 0, 
		STAT = 1,
		DATETIME = 2,
		TIME = 3
	};

	class RTC : public GpioDeviceBase {
	public :
		RTC();

		common::u8 Read() override;
		void Write(common::u8 pin_values) override;

		virtual GpioDevType GetDevType() const override {
			return GpioDevType::RTC;
		}

		~RTC() override = default;

		template <typename Ar>
		void save(Ar& ar) const {
			ar(m_communication_enable);
			ar(m_serial_clock);
			ar(m_serial_data);

			ar(m_in_buffer);
			ar(m_out_buffer);
			
			ar(m_status);
			ar(m_comm_dir);
			ar(m_command);

			ar(m_recv_bits);
			ar(m_processed_bits);
			ar(m_wanted_bits);

			ar(m_stat.raw);

			ar(m_time_latch.hour);
			ar(m_time_latch.minute);
			ar(m_time_latch.second);

			ar(m_date_latch.year);
			ar(m_date_latch.month);
			ar(m_date_latch.day_of_week);
			ar(m_date_latch.day);
		}

		template <typename Ar>
		void load(Ar& ar) {
			ar(m_communication_enable);
			ar(m_serial_clock);
			ar(m_serial_data);

			ar(m_in_buffer);
			ar(m_out_buffer);

			ar(m_status);
			ar(m_comm_dir);
			ar(m_command);

			ar(m_recv_bits);
			ar(m_processed_bits);
			ar(m_wanted_bits);

			ar(m_stat.raw);

			ar(m_time_latch.hour);
			ar(m_time_latch.minute);
			ar(m_time_latch.second);

			ar(m_date_latch.year);
			ar(m_date_latch.month);
			ar(m_date_latch.day_of_week);
			ar(m_date_latch.day);
		}

	private :
		void ProcessInput();

		bool Command_Stat();
		bool Command_Reset();
		bool Command_DayTime();
		bool Command_Time();

		void DateTimeLatch();

		bool m_communication_enable;
		bool m_serial_clock;
		bool m_serial_data;

		uint64_t m_in_buffer;
		uint64_t m_out_buffer;

		RtcStatus m_status;
		CommandDirection m_comm_dir;
		Command m_command;

		uint64_t m_recv_bits;
		uint64_t m_wanted_bits;

		uint64_t m_processed_bits;

		union {
			struct {
				bool : 1;
				bool irq_duty_hold : 1;
				bool : 1;
				bool per_minute_irq : 1;
				bool : 1;
				bool : 1;
				bool mode : 1;
				bool power_off : 1;
			};

			common::u8 raw;
		} m_stat;

		struct {
			common::u8 hour;
			common::u8 minute;
			common::u8 second;
		} m_time_latch;

		struct {
			common::u8 year;
			common::u8 month;
			common::u8 day;
			common::u8 day_of_week;
		} m_date_latch;

		static constexpr common::u8 STAT_MASK = 0b01101010;
	};
}