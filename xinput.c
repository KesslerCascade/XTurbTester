#include <Windows.h>
#include <XInput.h>
#include <stdio.h>
#include "xinput.h"
#include "analysis.h"
#include "ui.h"
#include "timestamp.h"

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
    HMODULE lib = LoadLibraryW(L"xinput1_4.dll");
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
    if (newState && !lastState) {
        analysisInput();        // send button press to be logged

        const char* bname = "Other Button";
        if (state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
            bname = "A Button";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_B)
            bname = "B Button";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
            bname = "X Button";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
            bname = "Y Button";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
            bname = "Right Shoulder";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
            bname = "Left Shoulder";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
            bname = "Right Thumb";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
            bname = "Left Thumb";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
            bname = "Back Button";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_START)
            bname = "Start Button";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
            bname = "D-Pad Right";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
            bname = "D-Pad Left";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
            bname = "D-Pad Down";
        else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
            bname = "D-Pad Up";
        uiSetLastInput(bname, tsGet());
    }
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