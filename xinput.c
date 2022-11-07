#include <Windows.h>
#include <XInput.h>
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

static CRITICAL_SECTION controllerLock;
static int controllerId = 1;

static DWORD lastPacket;
static bool lastState;

bool xinputInit(void)
{
    HMODULE lib = LoadLibrary(_T("xinput1_4.dll"));
    if (!lib)
        return false;

    dllXInputGetState = (dllXInputGetState_t)GetProcAddress(lib, "XInputGetState");
    dllXInputSetState = (dllXInputSetState_t)GetProcAddress(lib, "XInputSetState");
    dllXInputGetCapabilities = (dllXInputGetCapabilities_t)GetProcAddress(lib, "XInputGetCapabilities");
    dllXInputEnable = (dllXInputEnable_t)GetProcAddress(lib, "XInputEnable");

    InitializeCriticalSection(&controllerLock);

    return dllXInputGetState && dllXInputSetState && dllXInputGetCapabilities && dllXInputEnable;
}

void xinputEnable(void)
{
    EnterCriticalSection(&controllerLock);
    dllXInputEnable(TRUE);
    LeaveCriticalSection(&controllerLock);
}

void xinputDisable(void)
{
    EnterCriticalSection(&controllerLock);
    dllXInputEnable(FALSE);
    LeaveCriticalSection(&controllerLock);
}

void xinputSelect(int controller)
{
    EnterCriticalSection(&controllerLock);
    controllerId = controller;
    LeaveCriticalSection(&controllerLock);
}

void xinputPoll(void)
{
    XINPUT_STATE state;
    DWORD ret;

    // early out if the selected controller isn't an xinput one
    if (controllerId == 0)
        return;

    EnterCriticalSection(&controllerLock);
    ret = dllXInputGetState(controllerId - 1, &state);
    LeaveCriticalSection(&controllerLock);

    if (ret != ERROR_SUCCESS || state.dwPacketNumber == lastPacket)
        return;

    lastPacket = state.dwPacketNumber;
    // TODO: optional button filter
    bool newState = !!state.Gamepad.wButtons;
    if (newState && !lastState)
        analysisInput();        // send button press to be logged
    lastState = newState;
}