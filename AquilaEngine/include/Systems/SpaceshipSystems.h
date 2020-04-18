#pragma once
#include <PrecompiledHeader.h>

struct SpaceshipMovementSystem : public System {
	
	SpaceshipMovementSystem() {};

	virtual void update(ECS_GameWorld & world);
};
