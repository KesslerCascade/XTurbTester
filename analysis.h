#pragma once
#include "xturbtester.h"

// analysis thread
bool analysisInit(void);
void analysisStart(void);
void analysisStop(void);

// fire an input event
void analysisInput();
