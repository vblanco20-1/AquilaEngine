#pragma once

#include <PrecompiledHeader.h>
#include "ECSCore.h"

struct PlayerCameraSystem : public System {

	PlayerCameraSystem() { };

	virtual void update(ECS_GameWorld& world);
};
struct Level1Transform {};
struct Level2Transform {};
struct SpaceshipSpawnSystem : public System {

	SpaceshipSpawnSystem() {}
	

	struct SpawnUnit {
		XMFLOAT4 Position;
		XMFLOAT4 MoveTarget;
		XMFLOAT3 Color;
	};

	virtual void update(ECS_GameWorld & world);

	moodycamel::ConcurrentQueue<SpawnUnit> SpawnQueue;
};

using namespace moodycamel;
struct PlayerInputSystem : public System {

	PlayerInputSystem() { };
	
	virtual void update(ECS_GameWorld& world);
};
