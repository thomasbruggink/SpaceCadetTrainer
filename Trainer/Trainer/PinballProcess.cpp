#include "PinballProcess.h"
#include "PinballProcessEx.h"
#include "MemHelper.h"

#define PROCESS_NAME TEXT("pinball.exe")	
#define LEFT_FLIPPER 0x3E8;
#define RIGHT_FLIPPER 0x3ea;
#define PLUNGER 0x3ec;

PinballProcess::PinballProcess()
{
	_proc = nullptr;
	_injectedHandle = nullptr;
}

bool PinballProcess::Load()
{
	_proc = mem::GetProcessHandleByName(PROCESS_NAME);
	if (!_proc) {
		return false;
	}
	_injectedHandle = mem::InjectSelf(_proc);
	if (!_injectedHandle) {
		return false;
	}
	return true;
}

bool PinballProcess::GetDebugMode(bool& state)
{
	unsigned int size = 1;
	LPVOID buffer = new LPVOID[size]{ 0 };
	if (mem::ReadMemory(_proc->handle, DEBUG_MODE_ADDR, buffer, size) < size) {
		return false;
	}
	state = reinterpret_cast<char*>(buffer)[0] == 1;
	delete[] buffer;
	return true;
}

bool PinballProcess::SetDebugMode(bool state)
{
	unsigned int size = 1;
	char* buffer = new char[size] { state ? 1 : 0 };
	if (mem::WriteMemory(_proc->handle, DEBUG_MODE_ADDR, buffer, size) < size) {
		return false;
	}
	delete[] buffer;
	return true;
}

bool PinballProcess::GetTotalRunTime(float& runTime)
{
	unsigned int size = 4;
	LPVOID buffer = new LPVOID[size]{ 0 };
	if (mem::ReadMemory(_proc->handle, TOTAL_RUN_TIME_ADDR, buffer, size) < size) {
		return false;
	}
	auto* result = reinterpret_cast<float*>(buffer);
	runTime = *result;
	delete[] buffer;
	return true;
}

bool PinballProcess::GetLeftFlipperKey(int& key)
{
	unsigned int size = 4;
	LPVOID buffer = new LPVOID[size]{ 0 };
	if (mem::ReadMemory(_proc->handle, LEFT_FLIPPER_ADDR, buffer, size) < size) {
		return false;
	}
	auto* result = reinterpret_cast<int*>(buffer);
	key = *result;
	delete[] buffer;
	return true;
}

bool PinballProcess::GetRightFlipperKey(int& key)
{
	unsigned int size = 4;
	LPVOID buffer = new LPVOID[size]{ 0 };
	if (mem::ReadMemory(_proc->handle, RIGHT_FLIPPER_ADDR, buffer, size) < size) {
		return false;
	}
	auto* result = reinterpret_cast<int*>(buffer);
	key = *result;
	delete[] buffer;
	return true;
}

bool PinballProcess::LeftFlipper(bool pressed)
{
	HandleKeyInputData data;
	data.state = pressed;
	GetLeftFlipperKey(data.key);
	return mem::InvokeRemote(_proc, nullptr, "HandleKeyInputFunc", &data, sizeof(HandleKeyInputData), nullptr);
}

bool PinballProcess::RightFlipper(bool pressed)
{
	HandleKeyInputData data;
	data.state = pressed;
	GetRightFlipperKey(data.key);
	return mem::InvokeRemote(_proc, nullptr, "HandleKeyInputFunc", &data, sizeof(HandleKeyInputData), nullptr);
}

PinballProcess::~PinballProcess()
{
	if (_proc != nullptr) {
		CloseHandle(_proc);
		_proc = nullptr;
	}
	if (_injectedHandle != nullptr) {
		CloseHandle(_injectedHandle);
		_injectedHandle = nullptr;
	}
}
