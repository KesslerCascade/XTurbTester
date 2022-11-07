#include "poll.h"
#include "xinput.h"
#include <Windows.h>
#include <process.h>

typedef NTSTATUS(WINAPI* NtDelayExecution_t)(BOOL Alertable, PLARGE_INTEGER DelayInterval);
static NtDelayExecution_t pNtDelayExecution;
typedef NTSTATUS(WINAPI* NtSetTimerResolution_t)(IN ULONG RequestedResolution, IN BOOLEAN Set, OUT PULONG ActualResolution);
static NtSetTimerResolution_t pNtSetTimerResolution;

static volatile bool pollExit = false;

static unsigned pollThread(void* dummy)
{
	while (!pollExit) {
		LARGE_INTEGER interval;
		interval.QuadPart = -5000;				// 0.5ms
		pNtDelayExecution(false, &interval);	// equivalent to Sleep(0.5) if Sleep could take numbers <1

		xinputPoll();
	}

	return 0;
}

bool pollInit(void)
{
	HMODULE hMod = GetModuleHandle(_T("ntdll.dll"));
	if (hMod) {
		pNtDelayExecution = (NtDelayExecution_t)GetProcAddress(hMod, "NtDelayExecution");
		pNtSetTimerResolution = (NtSetTimerResolution_t)GetProcAddress(hMod, "NtSetTimerResolution");

		if (pNtDelayExecution && pNtSetTimerResolution) {
			// Use undocumented API to set timer resolution to the minimum (0.5ms).
			// This is basically the same as timeBeginPeriod but allows us to go below 1s
			// for maximum resolution when polling xinput.
			ULONG actualResolution;
			pNtSetTimerResolution(5000, true, &actualResolution);
			return true;
		}
	}

	return false;
}

void pollStart(void)
{
	if (!pollExit) {
		_beginthreadex(NULL, 0, pollThread, NULL, 0, NULL);
	}
}

void pollStop(void)
{
	pollExit = true;
}