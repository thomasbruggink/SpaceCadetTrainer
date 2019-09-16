#pragma once

#define DEBUG_MODE_ADDR (LPVOID)0x01024ff8
#define TOTAL_RUN_TIME_ADDR (LPVOID)0x01025630
#define LEFT_FLIPPER_ADDR (LPVOID)0x01028238
#define RIGHT_FLIPPER_ADDR (LPVOID)0x0102823c
#define HANDLE_KEY_DOWN_FUNC_ADDR (LPVOID)0x01015072
#define HANDLE_KEY_UP_FUNC_ADDR (LPVOID)0x010152e4
#define GLOBAL_HWND_ADDR (LPVOID)0x010281fc

typedef void HandleKeyDef(int virtualKey);

struct HandleKeyInputData {
	int key;
	bool state;
};

extern "C"
{
	__declspec(dllexport) void HandleKeyInputFunc(const void* data);
}