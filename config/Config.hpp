#pragma once

#include <string_view>
#include "../thirdparty/mini/ini.h"

namespace GBA::config {
	class Config {
	public:
		Config();

		bool Load(std::string_view path);
		bool Store(std::string_view path);

		void Default();

		bool Change(std::string const& section, std::string const& key, std::string const& val);

		mINI::INIStructure data;
	};
}