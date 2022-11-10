#include "input.h"
#include "xinput.h"
#include "rawinput.h"
#include <stdlib.h>

bool addInputSource(InputSourceList* list, InputType type, const wchar_t* name, unsigned long vendor, unsigned long product)
{
	InputSource* src = malloc(sizeof(InputSource));
	if (!src)
		return false;

	src->type = type;
	src->product = vendor;
	src->vendor = product;
	src->name = _wcsdup(name);
	if (!src->name) {
		free(src);
		return false;
	}

	InputSource** newsrc = realloc(list->src, ((size_t)list->count + 1) * sizeof(void*));
	if (!newsrc) {
		free(src->name);
		free(src);
		return false;
	}
	list->src = newsrc;
	list->src[list->count] = src;
	list->count++;

	return true;
}

InputSourceList* getInputSources(void)
{
	InputSourceList* list = calloc(1, sizeof(InputSourceList));
	if (!list)
		return NULL;

	list->src = calloc(1, sizeof(void*));

	xinputEnum(list);
	rawinputEnum(list);
	addInputSource(list, INPUT_KB, L"Keyboard", 0, 0);
	addInputSource(list, INPUT_NONE, L"None", 0, 0);

	return list;
}

void freeInputSources(InputSourceList* list)
{
	if (!(list && list->src))
		return;

	for (int i = 0; i < list->count; i++) {
		if (list->src[i]) {
			if (list->src[i]->name)
				free(list->src[i]->name);
			free(list->src[i]);
		}
	}

	free(list);
}