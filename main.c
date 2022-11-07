#include <windows.h>

#include "xturbtester.h"
#include "analysis.h"
#include "poll.h"
#include "timestamp.h"
#include "ui.h"
#include "xinput.h"

int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
	if (hPrevInstance)
		return 0;

	tsInit();

	if (!xinputInit()) {
		MessageBox(NULL, _T("Failed to initialize XInput, is it installed?"), PROGNAME_TCH, MB_ICONERROR | MB_OK);
	}

	if (!pollInit() || !analysisInit() || !uiInit(hInstance)) {
		MessageBox(NULL, _T("Unknown error"), PROGNAME_TCH, MB_ICONERROR | MB_OK);
	}

	// start background threads
	pollStart();
	analysisStart();

	uiRun(nCmdShow);

	analysisStop();
	pollStop();

	return 0;
}