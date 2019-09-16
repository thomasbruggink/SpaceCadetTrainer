#include "PinballProcessEx.h"
#include <Windows.h>

void HandleKeyInputFunc(const void* data)
{
	auto* keyData = (HandleKeyInputData*)data;
	// Up
	HandleKeyDef* handleKeyInput;
	if (!keyData->state)
		handleKeyInput = (HandleKeyDef*)HANDLE_KEY_UP_FUNC_ADDR;
	// Down
	else
		handleKeyInput = (HandleKeyDef*)HANDLE_KEY_DOWN_FUNC_ADDR;

	auto* key = &keyData->key;
	// Call
	__asm {
		mov         esi, esp
		mov         eax, dword ptr[key]
		mov         ecx, dword ptr[eax]
		push        ecx
		call        dword ptr[handleKeyInput]
	}
}
