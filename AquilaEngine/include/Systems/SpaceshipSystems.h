#pragma once
//#include <PrecompiledHeader.h>
import ecscore;
struct SpaceshipMovementSystem : public System {
	
	SpaceshipMovementSystem() {};

	virtual void update(ECS_GameWorldBase & world);
};
