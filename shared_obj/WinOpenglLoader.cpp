#include "WinOpenglLoader.hpp"

#include <Windows.h>

namespace GBA::shared_object {
	WindowsOpenglLoader::WindowsOpenglLoader() :
		SharedObject(), WGL_GetProcAddress(nullptr)
	{}

	bool WindowsOpenglLoader::Load(std::string_view location) {
		bool ok = SharedObject::Load(location);

		if(ok)
			WGL_GetProcAddress = (Procedure)::GetProcAddress((HMODULE)m_handle, "wglGetProcAddress");

		return ok;
	}

	Procedure WindowsOpenglLoader::GetProcAddress(std::string_view name) {
		using wgl_type = PROC(WINAPI*)(LPCSTR);

		Procedure procedure{ nullptr };

		procedure = ((wgl_type)WGL_GetProcAddress)(name.data());

		if (procedure == nullptr ||
			(procedure == (void*)0x1) || (procedure == (void*)0x2) || (procedure == (void*)0x3) ||
			(procedure == (void*)-1))
		{
			procedure = ::GetProcAddress((HMODULE)m_handle, name.data());
		}

		return procedure;
	}
}