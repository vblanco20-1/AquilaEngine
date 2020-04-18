#pragma once
#include <PrecompiledHeader.h>
#include "ECSCore.h"

#include "Systems/TransformSystems.h"



namespace ecs::system {
	struct RenderCore;
}


struct TaskStruct;
using namespace ecs::system;
class ECS_GameWorld {
public:
	

	~ECS_GameWorld();

	float debug_elapsed;
	int debugiterations{ 0 };

	TaskStruct* tasksystem;
	void initialize();

	void update_all(float dt);

	EngineTimeComponent GetTime();
	BenchmarkInfo AllBench;


	ecs::system::RenderCore* Renderer;

	ECSWorld registry_decs;

	std::vector<System*> Systems;

	struct PlayerInputSystem* PlayerInput_system;
	struct PlayerCameraSystem* PlayerCamera_system;
	struct SpaceshipSpawnSystem* SpaceshipSpawn_system;
	struct BoidHashSystem* BoidHash_system;
	struct SpaceshipMovementSystem* SpaceshipMovement_system;
	struct DestructionSystem* Destruction_system;
	
	struct RotatorSystem* Rotator_system;
	struct UpdateTransform* UpdateTransform_system;
	struct ExplosionFXSystem* Explosion_system;
};

static ECS_GameWorld *GameWorld{ nullptr };
