#pragma once
#include "PrecompiledHeader.h"

struct ApplicationInfo {

	float deltaTime;
	float averagedDeltaTime;
	int Drawcalls;
	float SimTime;
	float RenderTime;
	int TotalEntities;
	int BoidEntities;
};

void AppInfoUI(ApplicationInfo & info);
