#pragma once
#include <PrecompiledHeader.h>
#include "ECSCore.h"
#include "Timer.h"
#include "Systems/TransformSystems.h"

namespace ecs::system {
	struct RenderCore;
}


using namespace ecs::system;
class ECS_GameWorld {
public:
	

	~ECS_GameWorld();

	float debug_elapsed;
	int debugiterations{ 0 };
	ecs::TaskEngine task_engine{ 3/*std::thread::hardware_concurrency()*/ };
	void initialize();

	void update_all(float dt);

	EngineTimeComponent GetTime();
	BenchmarkInfo AllBench;


	ecs::system::RenderCore* Renderer;
	ECS_Registry registry_entt;
	ECSWorld registry_decs;
	//ECS_Registry render_registry;
	std::vector<System*> Systems;

	struct PlayerInputSystem* PlayerInput_system;
	struct PlayerCameraSystem* PlayerCamera_system;
	struct SpaceshipSpawnSystem* SpaceshipSpawn_system;
	struct BoidHashSystem* BoidHash_system;
	struct SpaceshipMovementSystem* SpaceshipMovement_system;
	struct DestructionSystem* Destruction_system;
	
	struct RotatorSystem* Rotator_system;
	struct UpdateTransform* UpdateTransform_system;
};

static ECS_GameWorld *GameWorld{ nullptr };
