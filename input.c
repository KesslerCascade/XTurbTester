#include "input.h"
#include "xinput.h"
#include "rawinput.h"
#include <stdlib.h>

static void addNoneSource(InputSourceList* list)
{
	// dummy entry at the end of the list
	InputSource* src = malloc(sizeof(InputSource));
	if (!src)
		return;

	src->type = INPUT_NONE;
	src->product = 0;
	src->vendor = 0;
	src->name = _wcsdup(L"None");
	if (!src->name) {
		free(src);
		return;
	}

	InputSource** newsrc = realloc(list->src, (list->count + 1) * sizeof(void*));
	if (!newsrc) {
		free(src->name);
		free(src);
		return;
	}
	list->src = newsrc;
	list->src[list->count] = src;
	list->count++;
}

InputSourceList* getInputSources(void)
{
	InputSourceList* list = calloc(1, sizeof(InputSourceList));
	if (!list)
		return NULL;

	list->src = calloc(1, sizeof(void*));

	xinputEnum(list);
	rawinputEnum(list);
	addNoneSource(list);

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