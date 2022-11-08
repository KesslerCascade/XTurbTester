#pragma once
#include "xturbtester.h"
#include "input.h"
#include <Windows.h>

bool rawinputInit(void);
void rawinputSelect(HWND hWnd, unsigned long vendor, unsigned long product);
bool rawinputHandleMsg(UINT uMsg, WPARAM wPara, LPARAM lParam);

void rawinputEnum(InputSourceList* list);