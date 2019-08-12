#pragma once

/*
Benchfiler -- Benchmark instrumentation

single-file header library

Design:
As low overhead as possible. This is intended for when you want to measure
a hot piece of code that is getting hit many times over the course of a frame.
This is intended for getting reasonable results for A/B'ing changes in a live
environment.

Why name it benchfiling?

I consider profiling to be coarsely measuring your code to help determine 
which sections could use improvement.

I consider benchmarking to be measuring a small section of code with high accuracy to 
see if changes made to it are effectively making it faster.

In a perfect world, we would move code we want to improve into separate program where we can
run it millions of times in an isolated environment. This isn't always particularly easy though.
I think the approach taken here provides functionality necessary to benchmark effectively in
within the live environment.

Note that profilers tend to not be good at benchmarking like this because they often are:
1. threadsafe
2. must dynamically allocate so they can keep track of all zones
3. may be communicating zones over the network
4. may be sample based, which may miss the code you are interested in entirely


* Only one region at a time.
* Not thread-safe.
* No support for nested calls.

API:
Begin();
End();
Report();


Implementation notes:
Windows only

*/

#include <stdint.h>
#include <stdio.h>
#include <windows.h> // part of windows.h

namespace benchfiler {

// accessed every Report()
uint64_t s_ticksPerMicrosecond;

// written to every End()
struct benchfiler_state {
	uint64_t beginTimeTicks; // TODO: might be able to turn this into a stack variable passed into End
	uint64_t count;
	uint64_t allDurationTicks;
	uint64_t fastestTicks;
	bool filled;
};
static_assert(sizeof(benchfiler_state) <= 64, "benchfiler_state too big"); // should be smaller than cache line

static benchfiler_state s_state;


inline uint64_t Sample () {
	LARGE_INTEGER elapsedMicros;
	QueryPerformanceCounter(&elapsedMicros);
	return elapsedMicros.QuadPart;
}

inline void Begin () {
	s_state.beginTimeTicks = Sample();
}

inline void End () {
	if (s_state.filled)
		return;

	const uint64_t endTimeTicks = Sample();
	const uint64_t durationTicks = endTimeTicks - s_state.beginTimeTicks;
	const uint64_t newAllDuration = s_state.allDurationTicks + durationTicks;

	// don't overflow anything
	if (newAllDuration < s_state.allDurationTicks || s_state.count == UINT64_MAX) {
		s_state.filled = true;
		return;
	}

	s_state.allDurationTicks = newAllDuration;

	if (durationTicks < s_state.fastestTicks || s_state.fastestTicks == 0)
		s_state.fastestTicks = durationTicks;
		
	++s_state.count;
}

void Initialize () {
	LARGE_INTEGER frequencySeconds;
	QueryPerformanceFrequency(&frequencySeconds);
	s_ticksPerMicrosecond = frequencySeconds.QuadPart / 1000000;
}

void Report () {
	const uint64_t avgTicks = s_state.allDurationTicks / s_state.count;
	const uint64_t avgMicros = avgTicks / s_ticksPerMicrosecond;
	const uint64_t fastestMicros = s_state.fastestTicks / s_ticksPerMicrosecond;

	printf(
		"lowfiling report\n"
		"    avg (micros):     %llu\n"
		"    fastest (micros): %llu\n"
		"    hit count:        %llu\n",
		avgMicros,
		fastestMicros,
		s_state.count
	);
	

	// clear
	s_state = benchfiler_state{};
}

} // namespace benchfiler
