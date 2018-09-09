#pragma once
#include <PrecompiledHeader.h>
#include "ECSCore.h"
#include "Timer.h"
struct RenderSystem;
class ECS_GameWorld {
public:

	~ECS_GameWorld();

	float debug_elapsed;
	int debugiterations{ 0 };
	ecs::TaskEngine task_engine{ 6/*std::thread::hardware_concurrency()*/ };
	void Initialize();

	void Update_All(float dt);
	BenchmarkInfo AllBench;


	RenderSystem* Renderer;
	ECS_Registry registry;
	ECS_Registry render_registry;
	std::vector<System*> Systems;
};

static ECS_GameWorld *GameWorld{ nullptr };
