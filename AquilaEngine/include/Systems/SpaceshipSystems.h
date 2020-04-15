#pragma once
#include <PrecompiledHeader.h>
#include "ECSCore.h"


struct SpaceshipMovementSystem : public System {
	float elapsed{ 0.0f };

	SpaceshipMovementSystem() {};

	virtual void update(ECS_GameWorld & world);

private:
	//entt::PersistentView<uint64_t, SpaceshipMovementComponent, TransformComponent> *posview;
	
};
struct BoidReferenceTag;

void update_ship_chunk(DataChunk* chnk, BoidReferenceTag& boidref, std::atomic<int>& count);