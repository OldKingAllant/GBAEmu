#include "SharedObject.hpp"

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
	#include <intrin.h>
	#include <Windows.h>
#endif

namespace GBA::shared_object {
	SharedObject::SharedObject() :
		m_handle(nullptr),
		m_module_name{}
	{}

	bool SharedObject::Load(std::string_view location) {
#ifdef WINDOWS_DLL
		m_handle = (ObjectHandle)LoadLibraryA(location.data());
#endif 
		
		m_module_name = location;

		return m_handle != nullptr;
	}

	bool SharedObject::Unload() {
#ifdef WINDOWS_DLL
		FreeLibrary((HMODULE)m_handle);
#endif 

		return true;
	}

	Procedure SharedObject::GetProcAddress(std::string_view name) {
		Procedure procedure{nullptr};

#ifdef WINDOWS_DLL
		procedure = ::GetProcAddress((HMODULE)m_handle, name.data());
#endif // WINDOWS_DLL

		return procedure;
	}

	SharedObject::~SharedObject() {
		Unload();
	}
}