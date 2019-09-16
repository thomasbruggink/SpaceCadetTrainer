#include "MemHelper.h"
#include <tchar.h>
#include <iostream>
#include <processthreadsapi.h>

using namespace std;

namespace mem {
	Proc* GetProcessHandleByName(const TCHAR* name) {
		auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (!snap) {
			printf("Unable to create snapshot\n");
			return nullptr;
		}

		PROCESSENTRY32 PE32{ 0 };
		PE32.dwSize = sizeof(PE32);

		auto proc = Process32First(snap, &PE32);
		while (proc) {
			printf("ExeName: %S\n", &(PE32.szExeFile));
			if (!_tcsicmp(PE32.szExeFile, name))
				break;
			proc = Process32Next(snap, &PE32);
		}
		CloseHandle(snap);

		if (!proc) {
			return nullptr;
		}

		auto* result = new Proc();
		result->handle = OpenProcess(PROCESS_ALL_ACCESS, false, PE32.th32ProcessID);
		result->id = PE32.th32ProcessID;
		return result;
	}

	SIZE_T ReadMemory(HANDLE procId, LPCVOID address, LPVOID data, SIZE_T size) {
		SIZE_T bytesRead;
		auto readResult = ReadProcessMemory(procId, address, data, size, &bytesRead);
		if (!readResult) {
			auto err = GetLastError();
			printf("ReadProcessMemory failed: 0x%08X\n", err);
			return -1;
		}
		return bytesRead;
	}

	SIZE_T WriteMemory(HANDLE procId, LPVOID address, LPCVOID data, SIZE_T size) {
		SIZE_T bytesWritten;
		auto writeResult = WriteProcessMemory(procId, address, data, size, &bytesWritten);
		if (!writeResult) {
			auto err = GetLastError();
			printf("WriteProcessMemory failed: %#08X\n", err);
			return -1;
		}
		return bytesWritten;
	}

	HMODULE InjectSelf(Proc* proc)
	{
		WCHAR fileName[MAX_PATH + 1];
		GetModuleFileName(NULL, fileName, MAX_PATH + 1);
		auto dataLen = _tcslen(fileName) * sizeof(WCHAR);
		DWORD out;
		if (!InvokeRemote(proc, TEXT("kernel32.dll"), "LoadLibraryW", fileName, dataLen, &out)) {
			printf("Unable to inject self\n");
			return nullptr;
		}
		return reinterpret_cast<HMODULE>(out);
	}

	bool InvokeRemote(Proc* proc, const TCHAR* moduleNameIn, LPCSTR methodName, LPCVOID data, SIZE_T size, DWORD* dwOut)
	{
		TCHAR moduleName[MAX_PATH]{ 0 };
		if (moduleNameIn == nullptr) {
			WCHAR fullPath[MAX_PATH + 1];
			WCHAR fileName[MAX_PATH + 1];
			WCHAR fileExt[MAX_PATH + 1];
			GetModuleFileName(GetModuleHandle(NULL), fullPath, MAX_PATH);
			_tsplitpath_s(fullPath, NULL, 0, NULL, 0, fileName, MAX_PATH, fileExt, MAX_PATH);
			_tcscat_s(moduleName, fileName);
			_tcscat_s(moduleName, fileExt);
		}
		else {
			_tcscat_s(moduleName, moduleNameIn);
		}
		auto methodAddr = GetProcAddressEx(proc, moduleName, methodName);
		auto* remoteParamAddr = VirtualAllocEx(proc->handle, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!remoteParamAddr) {
			auto err = GetLastError();
			printf("Unable to allocated remote memory: %#08X\n", err);
			return false;
		}
		if (WriteMemory(proc->handle, remoteParamAddr, data, size) < size) {
			return false;
		}
		auto remoteThreadHandle = CreateRemoteThread(proc->handle, NULL, 0, (LPTHREAD_START_ROUTINE)methodAddr, remoteParamAddr, NULL, NULL);
		if (!remoteThreadHandle) {
			auto err = GetLastError();
			printf("Unable to create remote thread: %#08X\n", err);
			return false;
		}
		DWORD dwLocal;
		auto success = false;
		while (GetExitCodeThread(remoteThreadHandle, &dwLocal)) {
			if (dwLocal != STILL_ACTIVE) {
				if (dwOut != nullptr) {
					*dwOut = dwLocal;
				}
				success = true;
				break;
			}
		}
		if (!VirtualFreeEx(proc->handle, remoteParamAddr, 0, MEM_RELEASE))
			printf("Unable to free virtual external memory: %#08X", GetLastError());
		return success;
	}

	HINSTANCE GetModuleHandleEx(Proc* proc, const TCHAR* moduleName)
	{
		MODULEENTRY32 me32{ 0 };
		me32.dwSize = sizeof(me32);

		HANDLE hSnap;
		do
		{
			hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, proc->id);
		} while (hSnap == INVALID_HANDLE_VALUE || GetLastError() == ERROR_BAD_LENGTH);

		auto ret = Module32First(hSnap, &me32);
		do
		{
			if (!_tcsicmp(moduleName, me32.szModule))
				break;
			ret = Module32Next(hSnap, &me32);
		} while (ret);

		if(hSnap)
			CloseHandle(hSnap);

		if (!ret)
			return nullptr;

		return me32.hModule;
	}

	LPCVOID GetProcAddressEx(Proc* proc, const TCHAR * moduleName, const char* procName)
	{
		auto* modBase = reinterpret_cast<BYTE*>(GetModuleHandleEx(proc, moduleName));
		if (!modBase)
			return nullptr;

		auto* peHeader = new BYTE[0x1000];
		if (!ReadMemory(proc->handle, modBase, peHeader, 0x1000)) {
			delete[] peHeader;
			return nullptr;
		}

		auto* pNT = reinterpret_cast<IMAGE_NT_HEADERS*>(peHeader + reinterpret_cast<IMAGE_DOS_HEADER*>(peHeader)->e_lfanew);
		auto* pExportEntry = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		if (!pExportEntry->Size) {
			delete[] peHeader;
			return nullptr;
		}

		auto* exportData = new BYTE[pExportEntry->Size];
		if (!ReadMemory(proc->handle, modBase + pExportEntry->VirtualAddress, exportData, pExportEntry->Size)) {
			delete[] exportData;
			delete[] peHeader;
			return nullptr;
		}

		auto* localBase = exportData - pExportEntry->VirtualAddress;
		auto* pExportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(exportData);

		auto Forward = [&](DWORD FuncRVA) -> LPCVOID
		{
			char pFullExport[MAX_PATH + 1]{ 0 };
			auto len = strlen(reinterpret_cast<char*>(localBase + FuncRVA));
			if (!len)
				return nullptr;
			memcpy(pFullExport, reinterpret_cast<char*>(localBase, FuncRVA), len);

			auto* pFuncName = strchr(pFullExport, '.');
			*(pFuncName++) = 0;
			if (*pFuncName == '#')
				pFuncName = reinterpret_cast<char*>(LOWORD(atoi(++pFuncName)));

			TCHAR modNameW[MAX_PATH + 1]{ 0 };
			size_t sizeOut = 0;
			mbstowcs_s(&sizeOut, modNameW, pFullExport, MAX_PATH);

			return GetProcAddressEx(proc, modNameW, pFuncName);
		};

		if ((reinterpret_cast<UINT_PTR>(procName) & 0xFFFFFF) <= MAXWORD)
		{
			auto base = LOWORD(pExportDir->Base - 1);
			auto ordinal = LOWORD(procName) - base;
			auto funcRva = reinterpret_cast<DWORD*>(localBase + pExportDir->AddressOfFunctions)[ordinal];

			delete[] exportData;
			delete[] peHeader;

			if (funcRva >= pExportEntry->VirtualAddress && funcRva < pExportEntry->VirtualAddress + pExportEntry->Size)
				return Forward(funcRva);

			return modBase + funcRva;
		}

		DWORD max = pExportDir->NumberOfNames - 1;
		DWORD min = 0;
		DWORD funcRva = 0;

		while (min <= max)
		{
			auto mid = (min + max) / 2;

			auto currNameRva = reinterpret_cast<DWORD*>(localBase + pExportDir->AddressOfNames)[mid];
			auto szName = reinterpret_cast<char*>(localBase + currNameRva);

			auto cmp = strcmp(szName, procName);
			if (cmp < 0)
				min = mid + 1;
			else if (cmp > 0)
				max = mid - 1;
			else {
				auto ordinal = reinterpret_cast<WORD*>(localBase + pExportDir->AddressOfNameOrdinals)[mid];
				funcRva = reinterpret_cast<DWORD*>(localBase + pExportDir->AddressOfFunctions)[ordinal];
				break;
			}
		}

		delete[] exportData;
		delete[] peHeader;

		if (!funcRva)
			return nullptr;

		if (funcRva >= pExportEntry->VirtualAddress && funcRva < pExportEntry->VirtualAddress + pExportEntry->Size)
			return Forward(funcRva);

		return modBase + funcRva;
	}
}