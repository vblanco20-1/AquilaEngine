#pragma once
#include <chrono>
struct BenchmarkInfo {

	const char* name;
	std::chrono::high_resolution_clock::time_point  stopwatch;
	float time;
	//std::chrono::time<long long, std::nano> time;
};

float Bench_GetMiliseconds(const BenchmarkInfo & bench);

void Bench_Start(BenchmarkInfo & bench);
void Bench_End(BenchmarkInfo & bench);