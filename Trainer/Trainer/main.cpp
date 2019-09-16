#include "PinballProcess.h"
#include <Windows.h>
#include <string>

int main()
{
	PinballProcess pinball;
	if (!pinball.Load())
		return 1;

	if (!pinball.SetDebugMode(true))
		return 1;

	bool result;
	if (pinball.GetDebugMode(result))
		printf("Debug mode: %s\n", result ? "true" : "false");
	else
		return 1;

	int sleepTime = 90;
	while (true) {
		printf("Hitting left flipper\n");
		if (!pinball.LeftFlipper(true))
			return 1;
		printf("Hitting right flipper\n");
		if (!pinball.RightFlipper(true))
			return 1;

		Sleep(sleepTime);

		printf("Releasing left flipper\n");
		if (!pinball.LeftFlipper(false))
			return 1;
		printf("Releasing right flipper\n");
		if (!pinball.RightFlipper(false))
			return 1;

		Sleep(sleepTime);
	}
}