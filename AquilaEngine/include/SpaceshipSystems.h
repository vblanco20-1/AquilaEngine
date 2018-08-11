#pragma once
#include <PrecompiledHeader.h>
#include "ECSCore.h"


struct SpaceshipMovementSystem : public System {
	float elapsed{ 0.0f };

	SpaceshipMovementSystem() { uses_threading = true; };
	virtual ecs::TaskEngine::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::TaskEngine::Task & parent);;

	virtual void update(ECS_Registry &registry, float dt);

	
};