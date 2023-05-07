#pragma once

#include <string_view>
#include <string>

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
	#define WINDOWS_DLL 1
#endif

namespace GBA::shared_object {
	using ObjectHandle = void*;
	using Procedure = void*;

	class SharedObject {
	public :
		SharedObject();

		bool Load(std::string_view location);
		bool Unload();

		Procedure GetProcAddress(std::string_view name);

		~SharedObject();

	private :
		ObjectHandle m_handle;
		Procedure WGL_GetProcAddress;
		std::string m_module_name;
	};
}