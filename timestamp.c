#include "timestamp.h"
#include <Windows.h>

static int64 qpcMult;
static int64 qpcDivisor;

static void qpcInit(void)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    // calculate what the converstion to microseconds will be

    if (freq.QuadPart > 1000000) {
        qpcMult = 10;
        qpcDivisor = freq.QuadPart / 100000;
    }
    else {
        qpcMult = 100000 / freq.QuadPart;
        qpcDivisor = 10;
    }
}

void tsInit(void)
{
    qpcInit();
}

int64 tsGet(void)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (int64)(counter.QuadPart * qpcMult / qpcDivisor);
}