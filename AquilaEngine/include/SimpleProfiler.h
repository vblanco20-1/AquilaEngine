#pragma once


//#include <PrecompiledHeader.h>
#include "Timer.h"
#include <string>
#include <vector>

struct SystemPerformanceUnit {

	std::string EventName;
	float miliseconds;
	uint32_t iterations;
	
};


struct SimpleProfiler {
	std::vector<SystemPerformanceUnit> units;
	std::vector<SystemPerformanceUnit> displayunits;
};
extern SimpleProfiler * g_SimpleProfiler;
struct ScopeProfiler {
	std::string  EventName;
	BenchmarkInfo bench;
	SimpleProfiler & profiler;
	ScopeProfiler(std::string _EventName, SimpleProfiler & _profiler) : EventName(_EventName), profiler(_profiler) {
		Bench_Start(bench);
	}
	~ScopeProfiler() {
		Bench_End(bench);

		SystemPerformanceUnit unit;
		unit.miliseconds = Bench_GetMiliseconds(bench);
		unit.EventName = EventName;
		unit.iterations = 1;

		bool bFound = false;
		for (auto& u : profiler.units){
			if (u.EventName == (EventName))
			{
				u.miliseconds += unit.miliseconds;
				u.iterations++;
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			profiler.units.push_back(unit);
		}
		

	}
};



void DrawSystemPerformanceUnits(std::vector<SystemPerformanceUnit>& units);

#define SCOPE_PROFILE(name) ScopeProfiler sc(name,*g_SimpleProfiler);

