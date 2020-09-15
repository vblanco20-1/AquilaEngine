#pragma once
#include <PrecompiledHeader.h>
#include <ECSCore.h>
struct SpaceshipMovementSystem : public System {
	
	SpaceshipMovementSystem() {};

	virtual void update(ECS_GameWorld & world);
};
