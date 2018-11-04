#pragma once
#include <PrecompiledHeader.h>
#include "ECSCore.h"
#include "Timer.h"

namespace ecs::system {
	struct RenderCore;
}



class ECS_GameWorld {
public:

	~ECS_GameWorld();

	float debug_elapsed;
	int debugiterations{ 0 };
	ecs::TaskEngine task_engine{ 6/*std::thread::hardware_concurrency()*/ };
	void initialize();

	void update_all(float dt);

	EngineTimeComponent GetTime();
	BenchmarkInfo AllBench;


	ecs::system::RenderCore* Renderer;
	ECS_Registry registry_entt;
	ECSWorld registry_decs;
	//ECS_Registry render_registry;
	std::vector<System*> Systems;
};

static ECS_GameWorld *GameWorld{ nullptr };
