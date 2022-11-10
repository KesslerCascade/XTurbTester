#pragma once
#include "xturbtester.h"

typedef enum
{
	INPUT_NONE = 0,
	INPUT_KB,
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
bool addInputSource(InputSourceList* list, InputType type, const wchar_t* name, unsigned long vendor, unsigned long product);
void freeInputSources(InputSourceList *list);