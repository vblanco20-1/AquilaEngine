#include <PrecompiledHeader.h>
#include "Timer.h"

float Bench_GetMiliseconds(const BenchmarkInfo & bench)
{
	
	return bench.time;
}

void Bench_Start(BenchmarkInfo & bench)
{
	bench.stopwatch = std::chrono::high_resolution_clock::now();
}

void Bench_End(BenchmarkInfo & bench)
{
	auto now = std::chrono::high_resolution_clock::now();

	bench.time = std::chrono::duration_cast<std::chrono::nanoseconds>(now - bench.stopwatch).count() * 0.000001f;
}
