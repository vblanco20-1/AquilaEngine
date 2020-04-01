#pragma once

#include <PrecompiledHeader.h>
#include "ECSCore.h"

struct PlayerCameraSystem : public System {

	PlayerCameraSystem() { uses_threading = true; };

	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent);;

	virtual void update(ECS_Registry& registry, float dt);
	virtual void update(ECS_GameWorld& world);
};
struct Level1Transform {};
struct Level2Transform {};
struct SpaceshipSpawnSystem : public System {

	SpaceshipSpawnSystem() { uses_threading = true; }
	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

		const float dt = get_delta_time(registry);
		ecs::Task task = task_engine.silent_emplace([&, dt]() {

			update(registry, dt);
		});


		task.name("Spaceship Spawn System");
		//run after the parent
		task.gather(parent);
		return std::move(task);
	};

	struct SpawnUnit {
		XMFLOAT4 Position;
		XMFLOAT4 MoveTarget;
		XMFLOAT3 Color;
	};

	virtual void update(ECS_Registry &registry, float dt);
	virtual void update(ECS_GameWorld & world);

	moodycamel::ConcurrentQueue<SpawnUnit> SpawnQueue;
};

using namespace moodycamel;
struct PlayerInputSystem : public System {

	PlayerInputSystem() { uses_threading = true; };
	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

		const float dt = get_delta_time(registry);
		ecs::Task task = task_engine.silent_emplace([&, dt]() {

			//update(registry, dt);
		});

		task.name("Player Input System");

		//run after the parent
		task.gather(parent);
		return std::move(task);
	};

	virtual void update(ECS_GameWorld& world);
};
