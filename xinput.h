#pragma once
#include "xturbtester.h"
#include "input.h"

bool xinputInit(void);
void xinputEnable(void);
void xinputDisable(void);
void xinputSelect(int controller);
void xinputPoll(void);

void xinputEnum(InputSourceList* list);