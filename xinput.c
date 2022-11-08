#include <Windows.h>
#include <XInput.h>
#include <stdio.h>
#include "xinput.h"
#include "analysis.h"

typedef DWORD(WINAPI* dllXInputGetState_t)(DWORD dwUserIndex, XINPUT_STATE* pState);
static dllXInputGetState_t dllXInputGetState;
typedef DWORD(WINAPI* dllXInputSetState_t)(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
static dllXInputSetState_t dllXInputSetState;
typedef DWORD(WINAPI* dllXInputGetCapabilities_t)(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);
static dllXInputGetCapabilities_t dllXInputGetCapabilities;
typedef DWORD(WINAPI* dllXInputEnable_t)(BOOL enable);
static dllXInputEnable_t dllXInputEnable;

static CRITICAL_SECTION xinputLock;
static int controllerId = 0;

static DWORD lastPacket;
static bool lastState;

static bool xinputAvail = false;

bool xinputInit(void)
{
    HMODULE lib = LoadLibrary(_T("xinput1_4.dll"));
    if (!lib)
        return false;

    dllXInputGetState = (dllXInputGetState_t)GetProcAddress(lib, "XInputGetState");
    dllXInputSetState = (dllXInputSetState_t)GetProcAddress(lib, "XInputSetState");
    dllXInputGetCapabilities = (dllXInputGetCapabilities_t)GetProcAddress(lib, "XInputGetCapabilities");
    dllXInputEnable = (dllXInputEnable_t)GetProcAddress(lib, "XInputEnable");

    InitializeCriticalSection(&xinputLock);

    xinputAvail = dllXInputGetState && dllXInputSetState && dllXInputGetCapabilities && dllXInputEnable;
    return xinputAvail;
}

void xinputEnable(void)
{
    if (!xinputAvail)
        return;

    EnterCriticalSection(&xinputLock);
    dllXInputEnable(TRUE);
    LeaveCriticalSection(&xinputLock);
}

void xinputDisable(void)
{
    if (!xinputAvail)
        return;

    EnterCriticalSection(&xinputLock);
    dllXInputEnable(FALSE);
    LeaveCriticalSection(&xinputLock);
}

void xinputSelect(int controller)
{
    EnterCriticalSection(&xinputLock);
    controllerId = controller;
    LeaveCriticalSection(&xinputLock);
}

void xinputPoll(void)
{
    if (!xinputAvail || controllerId == -1)
        return;

    XINPUT_STATE state;
    DWORD ret;

    EnterCriticalSection(&xinputLock);
    ret = dllXInputGetState(controllerId, &state);
    LeaveCriticalSection(&xinputLock);

    if (ret != ERROR_SUCCESS || state.dwPacketNumber == lastPacket)
        return;

    lastPacket = state.dwPacketNumber;
    // TODO: optional button filter
    bool newState = !!state.Gamepad.wButtons;
    if (newState && !lastState)
        analysisInput();        // send button press to be logged
    lastState = newState;
}

void xinputEnum(InputSourceList* list)
{
    if (!xinputAvail)
        return;

    // xinput supports 4 static controllers regardless if they exist or not
    wchar_t buf[22];
    for (int i = 0; i < 4; i++) {
        XINPUT_CAPABILITIES caps;

        if (dllXInputGetCapabilities(i, 0, &caps) == ERROR_DEVICE_NOT_CONNECTED)
            continue;

        _snwprintf_s(buf, 22, _TRUNCATE, L"XInput Controller #%d", i + 1);
        InputSource* src = malloc(sizeof(InputSource));
        if (!src)
            return;
        src->name = _wcsdup(buf);
        if (!src->name) {
            free(src);
            return;
        }

        src->type = INPUT_XINPUT;
        src->vendor = 0;
        src->product = i;

        InputSource** newsrc = realloc(list->src, (list->count + 1) * sizeof(void*));
        if (!newsrc) {
            free(src->name);
            free(src);
            return;
        }
        list->src = newsrc;
        list->src[list->count] = src;
        list->count++;
    }
}