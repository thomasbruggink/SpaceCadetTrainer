#pragma once

#include <Windows.h>

namespace mem {
	struct Proc;
}

class PinballProcess {
public:
	PinballProcess();
	bool Load();
	bool GetDebugMode(bool& state);
	bool SetDebugMode(bool state);
	bool GetTotalRunTime(float& runTime);
	bool GetLeftFlipperKey(int& key);
	bool GetRightFlipperKey(int& key);
	bool LeftFlipper(bool pressed);
	bool RightFlipper(bool pressed);
	~PinballProcess();
private:
	mem::Proc* _proc;
	HANDLE _injectedHandle;
};