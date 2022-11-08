#include "ui.h"
#include "xinput.h"
#include "rawinput.h"
#include "resource.h"
#include "timestamp.h"

#include <Windows.h>
#include <stdio.h>

// enable visual styles
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define XTWNDCLASS L"XTurbTesterClass"

static HWND hMainWin;
static HWND hControllerLabel;
static HWND hControllerSelect;
static HWND hControllerRefresh;
static HWND hControllerLastInput;

static HWND hRateSep;
static HWND hRateSepLabel;
static HWND hInstLabel;
static HWND hInstEdit;
static HWND hAvgLabel;
static HWND hAvgEdit;

static HWND hMeasuredSep;
static HWND hMeasuredSepLabel;
static HWND h1sLabel;
static HWND h2sLabel;
static HWND h3sLabel;
static HWND h5sLabel;
static HWND h10sLabel;
static HWND h1sEdit;
static HWND h2sEdit;
static HWND h3sEdit;
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
	idControllerSelect = 1001,
	idControllerRefresh
};

enum CustomMessage {
	MSG_FREQUPDATE = WM_USER + 1,
	MSG_MEASUREDUPDATE,
	MSG_DELAYUPDATE,
	MSG_SETLASTINPUT,
};

typedef struct FreqUpdateData
{
	double inst;
	double avg;
	int avgsec;
} FreqUpdateData;

typedef struct MeasuredUpdateData
{
	double s1;
	double s2;
	double s3;
	double s5;
	double s10;
} MeasuredUpdateData;

typedef struct DelayUpdateData
{
	int mind;
	int avgd;
	int maxd;
} DelayUpdateData;

typedef struct LastInputData
{
	char* name;
	int64 timestamp;
} LastInputData;

static bool lastinputShown;
static LastInputData* lastinput;

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

		uiSetLastInput(NULL, 0);
	}
}

static void refreshControllers()
{
	if (inputs)
		freeInputSources(inputs);

	SendMessageW(hControllerSelect, CB_RESETCONTENT, 0, 0);
	inputs = getInputSources();
	for (int i = 0; i < inputs->count; i++) {
		SendMessageW(hControllerSelect, CB_ADDSTRING, 0, (LPARAM)inputs->src[i]->name);
	}
	SendMessage(hControllerSelect, CB_SETCURSEL, 0, 0);
	selectController(hMainWin, 0);
}

static void uiCmd(HWND hWnd, int id, int cmd, LPARAM lparam)
{
	switch (id) {
	case idControllerSelect:
		if (cmd == CBN_SELCHANGE)
			selectController(hWnd, (int)SendMessage(hControllerSelect, CB_GETCURSEL, 0, 0));
		break;
	case idControllerRefresh:
		if (cmd == BN_CLICKED)
			refreshControllers();
		break;
	}
}

static void uiUpdateLastInput()
{
	int64 now = tsGet();
	if (lastinput && lastinput->name && now - lastinput->timestamp < 10000000) {
		char buf[256];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Last input: %s (%d msec ago)",
			lastinput->name, (int)((now - lastinput->timestamp) / 1000));
		SendMessageA(hControllerLastInput, WM_SETTEXT, 0, (LPARAM)buf);

		if (!lastinputShown) {
			ShowWindow(hControllerLastInput, SW_SHOW);
			lastinputShown = true;
		}
	}
	else {
		if (lastinputShown) {
			ShowWindow(hControllerLastInput, SW_HIDE);
			lastinputShown = false;
		}
	}
}

static void uiHandleFreqUpdate(FreqUpdateData* fd)
{
	char buf[20];

	snprintf(buf, sizeof(buf), "%0.4f", fd->inst);
	SendMessageA(hInstEdit, WM_SETTEXT, 0, (LPARAM)buf);
	snprintf(buf, sizeof(buf), "Avg (%d sec)", fd->avgsec);
	SendMessageA(hAvgLabel, WM_SETTEXT, 0, (LPARAM)buf);
	snprintf(buf, sizeof(buf), "%0.4f", fd->avg);
	SendMessageA(hAvgEdit, WM_SETTEXT, 0, (LPARAM)buf);

	free(fd);
}

static void uiHandleMeasuredUpdate(MeasuredUpdateData* fd)
{
	char buf[20];

	snprintf(buf, sizeof(buf), "%0.2f", fd->s1);
	SendMessageA(h1sEdit, WM_SETTEXT, 0, (LPARAM)buf);
	snprintf(buf, sizeof(buf), "%0.2f", fd->s2);
	SendMessageA(h2sEdit, WM_SETTEXT, 0, (LPARAM)buf);
	snprintf(buf, sizeof(buf), "%0.2f", fd->s3);
	SendMessageA(h3sEdit, WM_SETTEXT, 0, (LPARAM)buf);
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

	uiUpdateLastInput();
}

static void uiHandleSetLastInput(LastInputData* lid)
{
	if (lastinput) {
		if (lastinput->name)
			free(lastinput->name);
		free(lastinput);
	}

	lastinput = lid;
	uiUpdateLastInput();
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
	case MSG_MEASUREDUPDATE:
		uiHandleMeasuredUpdate((MeasuredUpdateData*)lParam);
		return 0;
	case MSG_DELAYUPDATE:
		uiHandleDelayUpdate((DelayUpdateData*)lParam);
		return 0;
	case MSG_SETLASTINPUT:
		uiHandleSetLastInput((LastInputData*)lParam);
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

	hMainWin = CreateWindowExW(WS_EX_CLIENTEDGE, XTWNDCLASS, PROGNAMEW,
		WS_CAPTION | WS_SYSMENU, 0, 0, 480, 400, NULL, NULL, hinst, NULL);

	int y = 20;
	hControllerLabel = CreateWindowExW(0, L"STATIC",
		L"Select controller:", WS_CHILD | WS_VISIBLE, 20, y, 160, 20,
		hMainWin, NULL, hinst, NULL);
	WPARAM hFont = (WPARAM)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hControllerLabel, WM_SETFONT, hFont, 0);
	y += 20;

	hControllerSelect = CreateWindowExW(0, L"COMBOBOX",
		L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 20, y, 300, 22,
		hMainWin, (HMENU)idControllerSelect, hinst, NULL);
	SendMessage(hControllerSelect, WM_SETFONT, hFont, 0);

	hControllerRefresh = CreateWindowExW(0, L"BUTTON",
		L"Refresh", WS_CHILD | WS_VISIBLE, 340, y, 60, 22,
		hMainWin, (HMENU)idControllerRefresh, hinst, NULL);
	SendMessage(hControllerRefresh, WM_SETFONT, hFont, 0);
	y += 25;

	hControllerLastInput = CreateWindowExW(0, L"STATIC",
		L"", WS_CHILD | WS_VISIBLE, 40, y, 300, 20,
		hMainWin, NULL, hinst, NULL);
	SendMessage(hControllerLastInput, WM_SETFONT, hFont, 0);

	refreshControllers();
    y += 40;

	hRateSep = CreateWindowExW(0, L"STATIC",
		NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
		130, y + 8, 310, 4,
		hMainWin, 0, hinst, NULL);
	hRateSepLabel = CreateWindowExW(0, L"STATIC",
		L"Calculated Rate (Hz)", WS_CHILD | WS_VISIBLE,
		20, y, 110, 32,
		hMainWin, 0, hinst, NULL);
	SendMessage(hRateSepLabel, WM_SETFONT, hFont, 0);
	y += 20;

	hInstLabel = CreateWindowExW(0, L"STATIC",
		L"Instant", WS_CHILD | WS_VISIBLE,
		40, y, 60, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hInstLabel, WM_SETFONT, hFont, 0);
	hAvgLabel = CreateWindowExW(0, L"STATIC",
		L"Avg (0 sec)", WS_CHILD | WS_VISIBLE,
		120, y, 80, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hAvgLabel, WM_SETFONT, hFont, 0);
	y += 20;
	hInstEdit = CreateWindowExW(0, L"EDIT",
		L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		40, y, 60, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hInstEdit, WM_SETFONT, hFont, 0);
	hAvgEdit = CreateWindowExW(0, L"EDIT",
		L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		120, y, 60, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hAvgEdit, WM_SETFONT, hFont, 0);
	y += 40;

	hMeasuredSep = CreateWindowExW(0, L"STATIC",
		NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
		130, y + 8, 310, 4,
		hMainWin, 0, hinst, NULL);
	hMeasuredSepLabel = CreateWindowExW(0, L"STATIC",
		L"Measured Rate (Hz)", WS_CHILD | WS_VISIBLE,
		20, y, 110, 32,
		hMainWin, 0, hinst, NULL);
	SendMessage(hMeasuredSepLabel, WM_SETFONT, hFont, 0);
	y += 20;

	h1sLabel = CreateWindowExW(0, L"STATIC",
		L"1 sec", WS_CHILD | WS_VISIBLE,
		40, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h1sLabel, WM_SETFONT, hFont, 0);
	h2sLabel = CreateWindowExW(0, L"STATIC",
		L"2 sec", WS_CHILD | WS_VISIBLE,
		120, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h2sLabel, WM_SETFONT, hFont, 0);
	h3sLabel = CreateWindowExW(0, L"STATIC",
		L"3 sec", WS_CHILD | WS_VISIBLE,
		200, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h3sLabel, WM_SETFONT, hFont, 0);
	h5sLabel = CreateWindowExW(0, L"STATIC",
		L"5 sec", WS_CHILD | WS_VISIBLE,
		280, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h5sLabel, WM_SETFONT, hFont, 0);
	h10sLabel = CreateWindowExW(0, L"STATIC",
		L"10 sec", WS_CHILD | WS_VISIBLE,
		360, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h10sLabel, WM_SETFONT, hFont, 0);
	y += 20;

	h1sEdit = CreateWindowExW(0, L"EDIT",
		L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		40, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h1sEdit, WM_SETFONT, hFont, 0);
	h2sEdit = CreateWindowExW(0, L"EDIT",
		L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		120, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h2sEdit, WM_SETFONT, hFont, 0);
	h3sEdit = CreateWindowExW(0, L"EDIT",
		L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		200, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h3sEdit, WM_SETFONT, hFont, 0);
	h5sEdit = CreateWindowExW(0, L"EDIT",
		L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		280, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h5sEdit, WM_SETFONT, hFont, 0);
	h10sEdit = CreateWindowExW(0, L"EDIT",
		L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		360, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(h10sEdit, WM_SETFONT, hFont, 0);
	y += 40;

	hDelaySep = CreateWindowExW(0, L"STATIC",
		NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
		180, y + 8, 260, 4,
		hMainWin, 0, hinst, NULL);
	hDelaySepLabel = CreateWindowExW(0, L"STATIC",
		L"Input Spacing (last 2 seconds)", WS_CHILD | WS_VISIBLE,
		20, y, 160, 32,
		hMainWin, 0, hinst, NULL);
	SendMessage(hDelaySepLabel, WM_SETFONT, hFont, 0);
	y += 20;

	hMinDelayLabel = CreateWindowExW(0, L"STATIC",
		L"Min (ms)", WS_CHILD | WS_VISIBLE,
		40, y, 80, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hMinDelayLabel, WM_SETFONT, hFont, 0);
	hAvgDelayLabel = CreateWindowExW(0, L"STATIC",
		L"Avg (ms)", WS_CHILD | WS_VISIBLE,
		120, y, 80, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hAvgDelayLabel, WM_SETFONT, hFont, 0);
	hMaxDelayLabel = CreateWindowExW(0, L"STATIC",
		L"Max (ms)", WS_CHILD | WS_VISIBLE,
		200, y, 80, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hMaxDelayLabel, WM_SETFONT, hFont, 0);
	y += 20;

	hMinDelayEdit = CreateWindowExW(0, L"EDIT",
		L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		40, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hMinDelayEdit, WM_SETFONT, hFont, 0);
	hAvgDelayEdit = CreateWindowExW(0, L"EDIT",
		L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		120, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hAvgDelayEdit, WM_SETFONT, hFont, 0);
	hMaxDelayEdit = CreateWindowExW(0, L"EDIT",
		L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
		200, y, 40, 20,
		hMainWin, 0, hinst, NULL);
	SendMessage(hMaxDelayEdit, WM_SETFONT, hFont, 0);
	y += 20;

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

void uiUpdateFreq(double inst, double avg, int avgsec)
{
	FreqUpdateData* d = malloc(sizeof(FreqUpdateData));
	if (!d)
		return;

	d->inst = inst;
	d->avg = avg;
	d->avgsec = avgsec;
	PostMessage(hMainWin, MSG_FREQUPDATE, 0, (LPARAM)d);
}


void uiUpdateMeasured(double s1, double s2, double s3, double s5, double s10)
{
	MeasuredUpdateData* d = malloc(sizeof(MeasuredUpdateData));
	if (!d)
		return;

	d->s1 = s1;
	d->s2 = s2;
	d->s3 = s3;
	d->s5 = s5;
	d->s10 = s10;
	PostMessage(hMainWin, MSG_MEASUREDUPDATE, 0, (LPARAM)d);
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

void uiSetLastInput(const char* name, int64 timestamp)
{
	LastInputData* d = malloc(sizeof(LastInputData));
	if (!d)
		return;

	d->name = name ? _strdup(name) : NULL;
	d->timestamp = timestamp;
	PostMessage(hMainWin, MSG_SETLASTINPUT, 0, (LPARAM)d);
}