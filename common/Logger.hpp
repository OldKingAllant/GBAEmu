#pragma once

#include <fmt/format.h>

#define LOG_CONTEXT(NAME) static const char* log_context_name = #NAME;

#define LOG_ERROR(STR, ...)  logging::Logger::Instance().LogError(log_context_name, STR, ##__VA_ARGS__)
#define LOG_WARNING(STR, ...)  logging::Logger::Instance().LogWarn(log_context_name, STR, ##__VA_ARGS__)
#define LOG_INFO(STR, ...)  logging::Logger::Instance().LogInfo(log_context_name, STR, ##__VA_ARGS__)

namespace GBA::logging {
	class Logger {
	public :
		template <typename StringType, typename... Args>
		void LogError(const char* context, StringType&& format_string, Args&&... args) {
			std::string buf = "[" + std::string(context) + "]"
				+ fmt::vformat(format_string, fmt::make_format_args(args...));
			log_impl(buf);
		}

		template <typename StringType, typename... Args>
		void LogWarning(const char* context, StringType&& format_string, Args&&... args) {
			std::string buf = "[" + std::string(context) + "]"
				+ fmt::vformat(format_string, fmt::make_format_args(args...) );
			log_impl(buf);
		}

		template <typename StringType, typename... Args>
		void LogInfo(const char* context, StringType&& format_string, Args&&... args) {
			std::string buf = "[" + std::string(context) + "]"
				+ fmt::vformat(format_string, fmt::make_format_args(args...));
			log_impl(buf);
		}

		static Logger& Instance() {
			static Logger logger;

			return logger;
		}

	private :
		Logger() = default;

		void log_impl(std::string const& str);
	};
}