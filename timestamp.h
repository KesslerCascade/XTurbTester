#pragma once
#include "xturbtester.h"

// Returns a timestamp from the monotonic clock.
// All measurements are in microseconds.

void tsInit(void);
int64 tsGet(void);