#pragma once
#include "xturbtester.h"
#include <Windows.h>

bool uiInit(HINSTANCE hinst);
void uiRun(int nCmdShow);

void uiSetLastInput(const wchar_t* name, int64 timestamp);
void uiUpdateFreq(double inst, double avg, int avgsec);
void uiUpdateMeasured(double s1, double s2, double s3, double s5, double s10);
void uiUpdateDelay(int mind, int avgd, int maxd);