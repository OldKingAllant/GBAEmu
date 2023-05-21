#pragma once

#include "SharedObject.hpp"

namespace GBA::shared_object {
	class WindowsOpenglLoader : public SharedObject {
	public :
		WindowsOpenglLoader();

		bool Load(std::string_view location) override;

		Procedure GetProcAddress(std::string_view name) override;

	private :
		Procedure WGL_GetProcAddress;
	};
}