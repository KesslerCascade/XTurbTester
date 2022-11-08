#include <windows.h>

#include "xturbtester.h"
#include "analysis.h"
#include "poll.h"
#include "timestamp.h"
#include "ui.h"
#include "xinput.h"
#include "rawinput.h"

int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
	if (hPrevInstance)
		return 0;

	tsInit();
	xinputInit();
	rawinputInit();

	if (!pollInit() || !analysisInit() || !uiInit(hInstance)) {
		MessageBoxW(NULL, L"Unknown error", PROGNAMEW, MB_ICONERROR | MB_OK);
	}

	// start background threads
	pollStart();
	analysisStart();

	uiRun(nCmdShow);

	analysisStop();
	pollStop();

	return 0;
}