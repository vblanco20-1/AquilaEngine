#include "Systems/SpaceshipSystems.h"
#include "Systems/BoidSystems.h"
#include "SimpleProfiler.h"
#include "taskflow/taskflow.hpp"

#include "GameWorld.h"

void UpdateSpaceship(SpaceshipMovementComponent & SpaceshipMov, TransformComponent & Transform, BoidReferenceTag & boidref, float DeltaTime);


ecs::Task SpaceshipMovementSystem::schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent)
{
	float dt = get_delta_time(registry);
	dt = 1.0 / 60.0;
	ecs::Task task = task_engine.silent_emplace([&, dt](auto& subflow) {
	
		rmt_ScopedCPUSample(SpaceshipMovementSystem, 0);
		SCOPE_PROFILE("Spaceship Movement System");


		BoidReferenceTag & boidref = registry.get<BoidReferenceTag>();

		//120 fps simulation
		//dt = 1.0 / 60.0;
		elapsed = dt;

		auto posview = registry.view<SpaceshipMovementComponent, TransformComponent>(entt::persistent_t{});

		auto[S, T] = parallel_for_ecs(subflow,
			posview,			
			[&](EntityID i,auto view) {			
			const auto entity = i;// view[i];
			UpdateSpaceship(view.get<SpaceshipMovementComponent>(entity), view.get<TransformComponent>(entity), boidref, dt);
			}
			,
			512  // execute one task at a time,
			,"Worker: Spaceship Movement"
			);
		//subflow.wait_for_all();


		//update(registry, dt);
	});
	

	//task_engine.linearize(parent, pre_task, S);

	//task.name("Spaceship Movement System");
	//run after the parent
	task.gather(parent);
	
	return std::move(task);
}

void UpdateSpaceship(SpaceshipMovementComponent & SpaceshipMov, TransformComponent & Transform, BoidReferenceTag & boidref, float DeltaTime)
{
	auto & ship = SpaceshipMov;//posview.get<SpaceshipMovementComponent>(entity);
	auto & t = Transform; // posview.get<TransformComponent>(entity);
		const float dt = DeltaTime;

	XMVECTOR Mov = ship.Target - t.position;
	Mov = XMVector3Normalize(Mov);
	Mov = Mov * ship.speed * dt;

	ship.Velocity += Mov;


	XMVECTOR OffsetVelocity{ 0.0f,0.0f,0.0f,0.0f };
	int num = 10;
	boidref.map->Foreach_EntitiesInRadius_Morton(3, t.position, [&](const GridItem2& boid) {
	
		XMVECTOR Avoidance = t.position - boid.pos;
		float dist = XMVectorGetX(XMVector3Length(Avoidance));
		OffsetVelocity += XMVector3Normalize(Avoidance)*  (1.0f - (std::clamp(dist / 10.0f, 0.0f, 1.0f)));
		num--;

	
	});



	ship.Velocity = XMVector3ClampLength(ship.Velocity + OffsetVelocity, 0.0f, ship.speed);

	XMMATRIX rotmat = XMMatrixLookAtLH(t.position, t.position + ship.Velocity * 10, XMVectorSet(0, 1, 0, 0));

	t.rotationQuat = XMQuaternionRotationMatrix(rotmat);

	t.position = t.position + ship.Velocity;
}

void SpaceshipMovementSystem::update(ECS_Registry &registry, float dt)
{
	rmt_ScopedCPUSample(SpaceshipMovementSystem, 0);
	SCOPE_PROFILE("Spaceship Movement System");
	

	BoidReferenceTag & boidref = registry.get<BoidReferenceTag>();

	//120 fps simulation
	dt = 1.0 / 60.0;
	elapsed = dt;

	auto  posview = registry.view<SpaceshipMovementComponent, TransformComponent>(entt::persistent_t{});

	std::for_each(/*std::execution::par_unseq, */posview.begin(), posview.end(), [&](const auto entity) {

		UpdateSpaceship(posview.get<SpaceshipMovementComponent>(entity), posview.get<TransformComponent>(entity), boidref, dt);
		
	});
	//}
}

static bool bEven = false;

void SpaceshipMovementSystem::update(ECS_GameWorld & world)
{
	rmt_ScopedCPUSample(SpaceshipMovementSystem, 0);
	SCOPE_PROFILE("Spaceship Movement System");

	Archetype SpaceshipTuple;
	SpaceshipTuple.AddComponent<SpaceshipMovementComponent>();
	SpaceshipTuple.AddComponent<TransformComponent>();


	Archetype CullTuple;
	CullTuple.AddComponent<Culled>();

	BoidReferenceTag & boidref = world.registry_entt.get<BoidReferenceTag>();

	world.registry_decs.IterateBlocks(SpaceshipTuple.componentlist, [&](ArchetypeBlock & block) {
		auto sparray = block.GetComponentArray<SpaceshipMovementComponent>();
		auto transfarray = block.GetComponentArray<TransformComponent>();
		for (int i = 0; i < block.last; i++)
		{
			int modulo = (block.entities[i].id % 2);
			if (bEven && modulo == 0)
			{
				UpdateSpaceship(sparray.Get(i), transfarray.Get(i), boidref, 1.0 / 30.f);
			}
			else if (modulo == 1)
			{
				UpdateSpaceship(sparray.Get(i), transfarray.Get(i), boidref, 1.0 / 30.f);
			}
			
		}

	}, true);

	bEven = !bEven;
}

