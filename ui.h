#pragma once
#include "xturbtester.h"
#include <Windows.h>

bool uiInit(HINSTANCE hinst);
void uiRun(int nCmdShow);

void uiUpdateFreq(double inst, double s1, double s2, double s5, double s10);
void uiUpdateDelay(int mind, int avgd, int maxd);