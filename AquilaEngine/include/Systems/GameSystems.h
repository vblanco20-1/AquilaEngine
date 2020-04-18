#pragma once

#include <PrecompiledHeader.h>


struct PlayerCameraSystem : public System {

	PlayerCameraSystem() { };

	virtual void update(ECS_GameWorld& world);
};

struct SpaceshipSpawnSystem : public System {

	SpaceshipSpawnSystem() {}

	struct SpawnUnit {
		XMFLOAT3 Position;
		XMFLOAT3 MoveTarget;
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


struct RotatorSystem : public System {

	RotatorSystem() {};

	virtual  void update(ECS_GameWorld& world);
};
