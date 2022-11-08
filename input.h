#pragma once
#include "xturbtester.h"

typedef enum
{
	INPUT_NONE = 0,
	INPUT_XINPUT,
	INPUT_RAWINPUT
} InputType;

typedef struct InputSource
{
	InputType type;
	unsigned long vendor;
	unsigned long product;
	wchar_t* name;
} InputSource;

typedef struct InputSourceList
{
	int count;
	InputSource** src;
} InputSourceList;

InputSourceList* getInputSources(void);
void freeInputSources(InputSourceList *list);