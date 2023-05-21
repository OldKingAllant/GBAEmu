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

		virtual bool Load(std::string_view location);
		virtual bool Unload();

		virtual Procedure GetProcAddress(std::string_view name);

		~SharedObject();

	protected :
		ObjectHandle m_handle;
		std::string m_module_name;
	};
}