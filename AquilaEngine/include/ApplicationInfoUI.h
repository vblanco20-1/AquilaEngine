#pragma once
#include <PrecompiledHeader.h>

struct ApplicationInfo {

	float deltaTime;
	float averagedDeltaTime;
	int Drawcalls;
	float SimTime;
	float RenderTime;
	int TotalEntities;
	int BoidEntities;

	float _SpawnRate = 1;
	bool bEnableCulling = true;
	bool bEnableBoids = true;
	bool bEnableSimulation = true;
};

void AppInfoUI(ApplicationInfo & info);
