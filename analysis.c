#include <process.h>
#include "analysis.h"
#include "ui.h"
#include "timestamp.h"

#define RBUF_GROW_SIZE 512

typedef struct RingBuffer {
	int64* buf;
	int capacity;
	int start;
	int end;
	CRITICAL_SECTION lock;
} RingBuffer;

static RingBuffer ebuf;			// buffer for incoming events
static RingBuffer abuf;			// analysis buffer

static volatile bool analysisExit = false;

static bool rbInit(RingBuffer* rb)
{
	InitializeCriticalSection(&rb->lock);
	rb->buf = malloc(RBUF_GROW_SIZE * sizeof(int64));
	if (!rb->buf)
		return false;

	rb->capacity = RBUF_GROW_SIZE;
	rb->start = rb->end = 0;
	return true;
}

static void rbLock(RingBuffer* rb)
{
	EnterCriticalSection(&rb->lock);
}

static void rbUnlock(RingBuffer* rb)
{
	LeaveCriticalSection(&rb->lock);
}

// ringbuffer functions should be called with the lock held!
static inline int rbLen(RingBuffer* rb)
{
	if (rb->end >= rb->start)
		return rb->end - rb->start;
	return rb->end + rb->capacity - rb->start;
}

static bool rbGrow(RingBuffer* rb)
{
	int newcap = rb->capacity + RBUF_GROW_SIZE;
	int64* newabuf = malloc(newcap * sizeof(int64));
	if (!newabuf)
		return false;

	int oldcount = rbLen(rb);
	if (rb->end > rb->start) {
		memcpy(newabuf, &rb->buf[rb->start], (rb->end - rb->start) * sizeof(int64));
	} else if (rb->end < rb->start) {
		memcpy(newabuf, &rb->buf[rb->start], (rb->capacity - rb->start) * sizeof(int64));
		memcpy(&newabuf[rb->capacity - rb->start], rb->buf, rb->end * sizeof(int64));
	}

	free(rb->buf);
	rb->start = 0;
	rb->end = oldcount;
	rb->buf = newabuf;
	rb->capacity = newcap;
	return true;
}

static bool rbAdd(RingBuffer* rb, int64 entry)
{
	int newend = (rb->end + 1) % rb->capacity;
	if (newend == rb->start) {
		if (!rbGrow(rb))
			return false;
	}
	rb->buf[rb->end] = entry;
	rb->end = newend;
	return true;
}

bool analysisInit(void)
{
	return rbInit(&ebuf) && rbInit(&abuf);
}

void analysisInput()
{
	rbLock(&ebuf);
	rbAdd(&ebuf, tsGet());
	rbUnlock(&ebuf);
}

static void analysisUpdate(int64 now)
{
	rbLock(&abuf);

	// copy new events from ebuf to abuf so we can unlock ebuf quickly
	rbLock(&ebuf);
	while (ebuf.start != ebuf.end) {
		rbAdd(&abuf, ebuf.buf[ebuf.start]);
		ebuf.start = (ebuf.start + 1) % ebuf.capacity;
	}
	rbUnlock(&ebuf);

	// The analysis is very basic for now.
	// Buckets to count the number of inputs within the last X seconds.
	int b1 = 0, b2 = 0, b5 = 0, b10 = 0;
	// Last seen timestamp and timestamp before that (to calculate instantaneous rate)
	int64 l1 = 0, l2 = 0;
	// min, max, total & count for calculating delay between inputs
	int64 mind = 0, maxd = 0, totald = 0;
	int countd = 0;

	int abp = abuf.start;			// ringbuffer pointer
	while (abp != abuf.end) {
		int64 ts = abuf.buf[abp];
		int64 age = now - ts;

		if (age > 10000000) {
			// more than 10s old, discard
			abuf.start = abp;
		} else {
			b10++;					// 10s bucket
			if (age < 5000000)
				b5++;				// 5s bucket
			if (age < 2000000)
				b2++;				// 2s bucket
			if (age < 1000000)
				b1++;				// 1s bucket

			if (l1 > 0 && age < 2000000) {
				int64 lage = ts - l1;
				if (mind == 0 || lage < mind)
					mind = lage;
				if (maxd == 0 || lage > maxd)
					maxd = lage;
				totald += lage;
				countd++;
			}

			l2 = l1;
			l1 = ts;
		}

		abp = (abp + 1) % abuf.capacity;
	}

	// done with abuf
	rbUnlock(&abuf);

	// update UI
	double instfreq = 0;
	// Instant freq is special.
	// Extrapolate it from the time elapsed between the last input and the one before that.
	if (l2 != 0 && now - l1 < 500000) {
		instfreq = 1000000. / (double)(l1 - l2);
	}
	// for frequency, just divide the number of inputs in each bucket by the length of time
	uiUpdateFreq(instfreq,
		(double)b1,
		(double)b2 / 2.,
		(double)b5 / 5.,
		(double)b10 / 10.);

	// delay counter - just divide by 1000 to get milliseconds
	uiUpdateDelay((int)(mind / 1000),
		(int)((countd > 0 ? totald / countd : 0) / 1000),
		(int)(maxd / 1000));
}

static unsigned analysisThread(void* dummy)
{
	while (!analysisExit) {
		Sleep(50);
		analysisUpdate(tsGet());
	}

	return 0;
}

void analysisStart()
{
	if (!analysisExit) {
		_beginthreadex(NULL, 0, analysisThread, NULL, 0, NULL);
	}
}

void analysisStop()
{
	analysisExit = true;
}