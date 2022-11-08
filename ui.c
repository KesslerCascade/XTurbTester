#include "ui.h"
#include "xinput.h"
#include "rawinput.h"
#include "resource.h"

#include <Windows.h>
#include <stdio.h>

// enable visual styles
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define XTWNDCLASS _T("XTurbTesterClass")

static HWND hMainWin;
static HWND hControllerLabel;
static HWND hControllerSelect;

static HWND hRateSep;
static HWND hRateSepLabel;
static HWND hInstLabel;
static HWND h1sLabel;
static HWND h2sLabel;
static HWND h5sLabel;
static HWND h10sLabel;
static HWND hInstEdit;
static HWND h1sEdit;
static HWND h2sEdit;
static HWND h5sEdit;
static HWND h10sEdit;

static HWND hDelaySep;
static HWND hDelaySepLabel;
static HWND hMinDelayLabel;
static HWND hAvgDelayLabel;
static HWND hMaxDelayLabel;
static HWND hMinDelayEdit;
static HWND hAvgDelayEdit;
static HWND hMaxDelayEdit;

static InputSourceList* inputs;

enum ControlIDs {
	idControllerSelect = 1001
};

enum CustomMessage {
	MSG_FREQUPDATE = WM_USER + 1,
	MSG_DELAYUPDATE
};

typedef struct FreqUpdateData
{
	double inst;
	double s1;
	double s2;
	double s5;
	double s10;
} FreqUpdateData;

typedef struct DelayUpdateData
{
	int mind;
	int avgd;
	int maxd;
} DelayUpdateData;

static void selectController(HWND hWnd, int cursel)
{
	if (cursel >= 0 && cursel < inputs->count) {
		InputSource* src = inputs->src[cursel];
		if (src->type == INPUT_XINPUT)
			xinputSelect(src->product);
		else
			xinputSelect(-1);

		if (src->type == INPUT_RAWINPUT)
			rawinputSelect(hWnd, src->vendor, src->product);
		else
			rawinputSelect(hWnd, 0, 0);
	}
}

static void uiCmd(HWND hWnd, int id, int cmd, LPARAM lparam)
{
	switch (id) {
	case idControllerSelect:
		if (cmd == CBN_SELCHANGE) {
			selectController(hWnd, (int)SendMessage(hControllerSelect, CB_GETCURSEL, 0, 0));
		}
		break;
	}
}

static void uiHandleFreqUpdate(FreqUpdateData* fd)
{
	char buf[20];

	snprintf(buf, sizeof(buf), "%0.3f", fd->inst);
	SendMessageA(hInstEdit, WM_SETTEXT, 0, (LPARAM)buf);
	snprintf(buf, sizeof(buf), "%0.2f", fd->s1);
	SendMessageA(h1sEdit, WM_SETTEXT, 0, (LPARAM)buf);
	snprintf(buf, sizeof(buf), "%0.2f", fd->s2);
	SendMessageA(h2sEdit, WM_SETTEXT, 0, (LPARAM)buf);
	snprintf(buf, sizeof(buf), "%0.2f", fd->s5);
	SendMessageA(h5sEdit, WM_SETTEXT, 0, (LPARAM)buf);
	snprintf(buf, sizeof(buf), "%0.2f", fd->s10);
	SendMessageA(h10sEdit, WM_SETTEXT, 0, (LPARAM)buf);

	free(fd);
}

static void uiHandleDelayUpdate(DelayUpdateData* fd)
{
	char buf[20];

	snprintf(buf, sizeof(buf), "%4d", fd->mind);
	SendMessageA(hMinDelayEdit, WM_SETTEXT, 0, (LPARAM)buf);
	snprintf(buf, sizeof(buf), "%4d", fd->avgd);
	SendMessageA(hAvgDelayEdit, WM_SETTEXT, 0, (LPARAM)buf);
	snprintf(buf, sizeof(buf), "%4d", fd->maxd);
	SendMessageA(hMaxDelayEdit, WM_SETTEXT, 0, (LPARAM)buf);

	free(fd);
}

static LRESULT CALLBACK uiProc(HWND hWnd, UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_ACTIVATE:
		if (wParam == WA_INACTIVE)
			xinputDisable();
		else
			xinputEnable();
		break;
	case WM_COMMAND:
		uiCmd(hWnd, LOWORD(wParam), HIWORD(wParam), lParam);
		return 0;
	case WM_INPUT:
		rawinputHandleMsg(msg, wParam, lParam);
		break;
	case MSG_FREQUPDATE:
		uiHandleFreqUpdate((FreqUpdateData*)lParam);
		return 0;
	case MSG_DELAYUPDATE:
		uiHandleDelayUpdate((DelayUpdateData*)lParam);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool uiInit(HINSTANCE hinst)
{
	WNDCLASSEX wc = { 0 };

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = uiProc;
	wc.hIcon = LoadIconW(hinst, MAKEINTRESOURCEW(IDI_APPICON));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszClassName = XTWNDCLASS;
	if (!RegisterClassEx(&wc))
		return false;

	hMainWin = CreateWindowEx(WS_EX_CLIENTEDGE, XTWNDCLASS, PROGNAME_TCH,
		WS_CAPTION | WS_SYSMENU, 0, 0, 480, 300, NULL, NULL, hinst, NULL);

	hControllerLabel = CreateWindowEx(0, _T("STATIC"),
		_T("Select controller:"), WS_CHILD | WS_VISIBLE, 20, 20, 160, 32,
		hMainWin, NULL, hinst, NULL);
	WPARAM hFont = (WPARAM)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hControllerLabel, WM_SETFONT, hFont, 0);

	hControllerSelect = CreateWindowEx(0, _T("COMBOBOX"),
		_T(""), WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 20, 40, 160, 32,
		hMainWin, (HMENU)idControllerSelect, hinst, NULL);
	SendMessage(hControllerSelect, WM_SETFONT, hFont, 0);

	inputs = getInputSources();
	for (int i = 0; i < inputs->count; i++) {
		SendMessageW(hControllerSelect, CB_ADDSTRING, 0, (LPARAM)inputs->src[i]->name);
	}
	SendMessage(hControllerSelect, CB_SETCURSEL, 0, 0);
	selectController(hMainWin, 0);

	hRateSep = CreateWindowEx(0, _T("STATIC"),
		NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
		110, 88, 330, 4,
		hMainWin, 0, hinst, NULL);
	hRateSepLabel = CreateWindowEx(0, _T("STATIC"),
		_T("Turbo Rate (Hz)"), WS_CHILD | WS_VISIBLE,
		20, 80, 90, 32,
		hMainWin, 0, hinst, NULL);
	SendMessage(hRateSepLabel, WM_SETFONT, hFont, 0);

	hInstLabel = CreateWindowEx(0, _T("STATIC"),
		_T("Instant"), WS_CHILD | WS_VISIBLE,
		40, 100, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hInstLabel, WM_SETFONT, hFont, 0);
	h1sLabel = CreateWindowEx(0, _T("STATIC"),
		_T("1 sec"), WS_CHILD | WS_VISIBLE,
		100, 100, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h1sLabel, WM_SETFONT, hFont, 0);
	h2sLabel = CreateWindowEx(0, _T("STATIC"),
		_T("2 sec"), WS_CHILD | WS_VISIBLE,
		160, 100, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h2sLabel, WM_SETFONT, hFont, 0);
	h5sLabel = CreateWindowEx(0, _T("STATIC"),
		_T("5 sec"), WS_CHILD | WS_VISIBLE,
		220, 100, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h5sLabel, WM_SETFONT, hFont, 0);
	h10sLabel = CreateWindowEx(0, _T("STATIC"),
		_T("10 sec"), WS_CHILD | WS_VISIBLE,
		280, 100, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h10sLabel, WM_SETFONT, hFont, 0);

	hInstEdit = CreateWindowEx(0, _T("EDIT"),
		_T("0"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		40, 120, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hInstEdit, WM_SETFONT, hFont, 0);
	h1sEdit = CreateWindowEx(0, _T("EDIT"),
		_T("0"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		100, 120, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h1sEdit, WM_SETFONT, hFont, 0);
	h2sEdit = CreateWindowEx(0, _T("EDIT"),
		_T("0"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		160, 120, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h2sEdit, WM_SETFONT, hFont, 0);
	h5sEdit = CreateWindowEx(0, _T("EDIT"),
		_T("0"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		220, 120, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h5sEdit, WM_SETFONT, hFont, 0);
	h10sEdit = CreateWindowEx(0, _T("EDIT"),
		_T("0"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		280, 120, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h10sEdit, WM_SETFONT, hFont, 0);

	hDelaySep = CreateWindowEx(0, _T("STATIC"),
		NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
		110, 168, 330, 4,
		hMainWin, 0, hinst, NULL);
	hDelaySepLabel = CreateWindowEx(0, _T("STATIC"),
		_T("2 second window"), WS_CHILD | WS_VISIBLE,
		20, 160, 90, 32,
		hMainWin, 0, hinst, NULL);
	SendMessage(hDelaySepLabel, WM_SETFONT, hFont, 0);

	hMinDelayLabel = CreateWindowEx(0, _T("STATIC"),
		_T("Min (ms)"), WS_CHILD | WS_VISIBLE,
		40, 180, 80, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hMinDelayLabel, WM_SETFONT, hFont, 0);
	hAvgDelayLabel = CreateWindowEx(0, _T("STATIC"),
		_T("Avg (ms)"), WS_CHILD | WS_VISIBLE,
		140, 180, 80, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hAvgDelayLabel, WM_SETFONT, hFont, 0);
	hMaxDelayLabel = CreateWindowEx(0, _T("STATIC"),
		_T("Max (ms)"), WS_CHILD | WS_VISIBLE,
		240, 180, 80, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hMaxDelayLabel, WM_SETFONT, hFont, 0);

	hMinDelayEdit = CreateWindowEx(0, _T("EDIT"),
		_T("0"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		40, 200, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hMinDelayEdit, WM_SETFONT, hFont, 0);
	hAvgDelayEdit = CreateWindowEx(0, _T("EDIT"),
		_T("0"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		140, 200, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hAvgDelayEdit, WM_SETFONT, hFont, 0);
	hMaxDelayEdit = CreateWindowEx(0, _T("EDIT"),
		_T("0"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		240, 200, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hMaxDelayEdit, WM_SETFONT, hFont, 0);

	return true;
}

void uiRun(int nCmdShow)
{
	ShowWindow(hMainWin, nCmdShow);

	MSG msg;
	BOOL ret;

	while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (ret == -1)
			break;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

// These functions exist so that the UI can be updated by any thread.
// They allocate memory and post a custom message to the main window, so that
// the main (UI) thread can pick it up and do synchronous SendMessages there.

void uiUpdateFreq(double inst, double s1, double s2, double s5, double s10)
{
	FreqUpdateData* d = malloc(sizeof(FreqUpdateData));
	if (!d)
		return;

	d->inst = inst;
	d->s1 = s1;
	d->s2 = s2;
	d->s5 = s5;
	d->s10 = s10;
	PostMessage(hMainWin, MSG_FREQUPDATE, 0, (LPARAM)d);
}

void uiUpdateDelay(int mind, int avgd, int maxd)
{
	DelayUpdateData* d = malloc(sizeof(DelayUpdateData));
	if (!d)
		return;

	d->mind = mind;
	d->avgd = avgd;
	d->maxd = maxd;
	PostMessage(hMainWin, MSG_DELAYUPDATE, 0, (LPARAM)d);
}