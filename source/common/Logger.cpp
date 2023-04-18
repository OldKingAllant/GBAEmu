#include "../../common/Logger.hpp"

#include <iostream>

namespace GBA::logging {
	void Logger::log_impl(std::string const& str) {
		std::cout << str << std::endl;
	}
}