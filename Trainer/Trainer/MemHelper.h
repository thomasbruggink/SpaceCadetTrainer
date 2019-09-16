#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>

namespace mem {
	struct Proc {
		HANDLE handle;
		DWORD id;
	};

	Proc* GetProcessHandleByName(const TCHAR* name);
	SIZE_T ReadMemory(HANDLE procId, LPCVOID address, LPVOID data, SIZE_T size);
	SIZE_T WriteMemory(HANDLE procId, LPVOID address, LPCVOID data, SIZE_T size);
	HMODULE InjectSelf(Proc* procId);
	bool InvokeRemote(Proc* proc, const TCHAR* moduleNameIn, LPCSTR methodName, LPCVOID data, SIZE_T size, DWORD* dwOut);
	HINSTANCE GetModuleHandleEx(Proc* proc, const TCHAR * moduleName);
	LPCVOID GetProcAddressEx(Proc* proc, const TCHAR * moduleName, const char* procName);
}