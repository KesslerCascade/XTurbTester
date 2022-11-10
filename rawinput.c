#include "rawinput.h"
#include "analysis.h"
#include "ui.h"
#include "timestamp.h"

#include <hidusage.h>
#include <hidpi.h>
#include <hidsdi.h>
#include <stdio.h>

#pragma comment(lib, "hid.lib")

static CRITICAL_SECTION rawinputLock;
static unsigned long riVendor;
static unsigned long riProduct;
static HANDLE riDevice;
static PHIDP_PREPARSED_DATA riPPData;
static int riButtonPage;
static int riButtonMin;
static int riButtons;
static bool riRegistered;

static bool lastState;

bool rawinputInit(void)
{
    InitializeCriticalSection(&rawinputLock);
    return true;
}

void rawinputSelect(HWND hWnd, unsigned long vendor, unsigned long product)
{
    EnterCriticalSection(&rawinputLock);
    if (riVendor != vendor || riProduct != product)
        riDevice = NULL;            // clear cached device
    riVendor = vendor;
    riProduct = product;

    if (riVendor == 0 && riProduct == 0) {
        // raw input should be deactivated
        if (riRegistered) {
            RAWINPUTDEVICE rid[2];

            rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
            rid[0].usUsage = HID_USAGE_GENERIC_JOYSTICK;
            rid[0].dwFlags = RIDEV_REMOVE;
            rid[0].hwndTarget = NULL;
            rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
            rid[1].usUsage = HID_USAGE_GENERIC_GAMEPAD;
            rid[1].dwFlags = RIDEV_REMOVE;
            rid[1].hwndTarget = NULL;

            RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE));
            riRegistered = false;
        }
    } else {
        // raw input should be activated
        if (!riRegistered) {
            RAWINPUTDEVICE rid[2];

            rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
            rid[0].usUsage = HID_USAGE_GENERIC_JOYSTICK;
            rid[0].dwFlags = 0;
            rid[0].hwndTarget = hWnd;
            rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
            rid[1].usUsage = HID_USAGE_GENERIC_GAMEPAD;
            rid[1].dwFlags = 0;
            rid[1].hwndTarget = hWnd;

            if (RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE)))
                riRegistered = true;
        }
    }

    LeaveCriticalSection(&rawinputLock);
}

static void rawinputProcessHID(BYTE* rawData, DWORD size)
{
    ULONG nusages;
    USAGE btnusage[256];
    int lastbutton = -1;

    nusages = riButtons;
    bool newState = false;
    if (HidP_GetUsages(HidP_Input, riButtonPage, 0, btnusage, &nusages, riPPData, rawData, size) == HIDP_STATUS_SUCCESS) {
        // got button data
        // TODO: button filter support
        for (int i = 0; i < (int)nusages; i++) {
            newState |= btnusage[i];
            if (btnusage[i])
                lastbutton = btnusage[i] - riButtonMin;
        }
    }

    if (newState && !lastState) {
        analysisInput();        // send button press to be logged

        wchar_t buf[64];
        _snwprintf_s(buf, 64, _TRUNCATE, L"Button %d", lastbutton + 1);
        uiSetLastInput(buf, tsGet());
    }
    lastState = newState;
}

static void rawinputProcess(RAWINPUT* rin)
{
    if (rin->header.dwType != RIM_TYPEHID)
        return;

    if (riDevice && rin->header.hDevice != riDevice)
        return;         // not the currently selected device

    if (!riDevice) {
        // do not have a selected device yet, need to populate capabilities
        RID_DEVICE_INFO rinfo = { 0 };
        UINT sz = sizeof(rinfo);
        if (GetRawInputDeviceInfo(rin->header.hDevice, RIDI_DEVICEINFO, &rinfo, &sz) <= 0)
            return;

        // check for the vendor & product we've been told to use
        if (rinfo.hid.dwVendorId != riVendor || rinfo.hid.dwProductId != riProduct)
            return;     // message is for some other device

        // the only device capability we really care about is how many buttons there are,
        // but have to decode the preparsed data to find that out
        sz = 0;
        if (GetRawInputDeviceInfo(rin->header.hDevice, RIDI_PREPARSEDDATA, NULL, &sz) != 0)
            return;

        if (riPPData)
            free(riPPData);
        riPPData = malloc(sz);
        if (!riPPData)
            return;
        if (GetRawInputDeviceInfo(rin->header.hDevice, RIDI_PREPARSEDDATA, riPPData, &sz) <= 0)
            return;

        HIDP_CAPS caps;
        if (HidP_GetCaps(riPPData, &caps) != HIDP_STATUS_SUCCESS)
            return;

        USHORT capslen = caps.NumberInputButtonCaps;
        HIDP_BUTTON_CAPS *buttonCaps = malloc(sizeof(HIDP_BUTTON_CAPS) * capslen);
        if (!buttonCaps)
            return;
        if (HidP_GetButtonCaps(HidP_Input, buttonCaps, &capslen, riPPData) != HIDP_STATUS_SUCCESS) {
            free(buttonCaps);
            return;
        }

        riButtonPage = buttonCaps->UsagePage;
        riButtonMin = buttonCaps->Range.UsageMin;
        riButtons = buttonCaps->Range.UsageMax - buttonCaps->Range.UsageMin + 1;

        free(buttonCaps);

        // everything checks out, remember that this is the right device
        riDevice = rin->header.hDevice;
    }

    rawinputProcessHID(rin->data.hid.bRawData, rin->data.hid.dwSizeHid);
}

bool rawinputHandleMsg(UINT uMsg, WPARAM wPara, LPARAM lParam)
{
    bool ret = false;
    UINT bufsz = 0;
    RAWINPUT* buf;

    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufsz, sizeof(RAWINPUTHEADER));
    if (bufsz == 0)
        return false;

    buf = malloc(bufsz);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buf, &bufsz, sizeof(RAWINPUTHEADER)) != (UINT)-1)
    {
        EnterCriticalSection(&rawinputLock);
        rawinputProcess(buf);
        LeaveCriticalSection(&rawinputLock);
        ret = true;
    }

    free(buf);
    return ret;
}

void rawinputEnum(InputSourceList* list)
{
    wchar_t buf[256];
    UINT ndevices = 0;
    RAWINPUTDEVICELIST* ridevices;
    GetRawInputDeviceList(NULL, &ndevices, sizeof(RAWINPUTDEVICELIST));

    if (ndevices == 0)
        return;

    ridevices = malloc(ndevices * sizeof(RAWINPUTDEVICELIST));
    if (!ridevices)
        return;

    GetRawInputDeviceList(ridevices, &ndevices, sizeof(RAWINPUTDEVICELIST));

    for (int i = 0; i < (int)ndevices; i++) {
        if (ridevices[i].dwType != RIM_TYPEHID)
            continue;

        bool havename = false;
        RID_DEVICE_INFO devinfo;
        UINT sz;
        devinfo.cbSize = sizeof(RID_DEVICE_INFO);
        sz = sizeof(devinfo);
        GetRawInputDeviceInfo(ridevices[i].hDevice, RIDI_DEVICEINFO, &devinfo, &sz);
        if (devinfo.hid.usUsagePage != HID_USAGE_PAGE_GENERIC ||
            (devinfo.hid.usUsage != HID_USAGE_GENERIC_JOYSTICK && devinfo.hid.usUsage != HID_USAGE_GENERIC_GAMEPAD))
            continue;

        sz = 256;
        buf[0] = 0;
        if (GetRawInputDeviceInfoW(ridevices[i].hDevice, RIDI_DEVICENAME, buf, &sz)) {
            if (wcsstr(buf, L"IG_"))
                continue;           // this is an XInput device, ignore it

            HANDLE hfile = CreateFileW(buf, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hfile != INVALID_HANDLE_VALUE) {
                if (HidD_GetProductString(hfile, buf, sizeof(buf)) && wcslen(buf) > 0)
                    havename = true;
                CloseHandle(hfile);
            }
        }

        if (!havename)
            // fallback for if we failed to read the device name
            _snwprintf_s(buf, 256, _TRUNCATE, L"HID Device (Vendor=%04x, Product=%04x)", devinfo.hid.dwVendorId, devinfo.hid.dwProductId);

        addInputSource(list, INPUT_RAWINPUT, buf, devinfo.hid.dwVendorId, devinfo.hid.dwProductId);
    }
    free(ridevices);
}