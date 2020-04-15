#include "Systems/SpaceshipSystems.h"
#include "Systems/BoidSystems.h"
#include "SimpleProfiler.h"
#include "taskflow/taskflow.hpp"

#include "GameWorld.h"
#include "ApplicationInfoUI.h"

void UpdateSpaceship(SpaceshipMovementComponent & SpaceshipMov, TransformComponent & Transform, BoidReferenceTag & boidref, float DeltaTime);


ecs::Task SpaceshipMovementSystem::schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent)
{
	float dt = get_delta_time(registry);
	dt = 1.0 / 60.0;
	ecs::Task task = task_engine.silent_emplace([&, dt](auto& subflow) {
	
		ZoneNamed(SpaceshipMovementSystem, true);
		SCOPE_PROFILE("Spaceship Movement System");


		BoidReferenceTag & boidref = registry.get<BoidReferenceTag>();

		//120 fps simulation
		//dt = 1.0 / 60.0;
		elapsed = dt;

		auto posview = registry.view<SpaceshipMovementComponent, TransformComponent>(entt::persistent_t{});

		auto[S, T] = parallel_for_ecs(subflow,
			posview,			
			[&](EntityID_entt i,auto view) {			
			const auto entity = i;// view[i];
			//UpdateSpaceship(view.get<SpaceshipMovementComponent>(entity), view.get<TransformComponent>(entity), boidref, dt);
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
			OffsetVelocity += XMVector3Normalize(Avoidance) * (1.0f - (std::clamp(dist / 10.0f, 0.0f, 1.0f)));
			num--;
			return num > 0;
	});

	OffsetVelocity *= 10;

	ship.Velocity = XMVector3ClampLength(ship.Velocity + OffsetVelocity, 0.0f, ship.speed);

	XMMATRIX rotmat = XMMatrixLookAtLH(t.position, t.position + ship.Velocity * 10, XMVectorSet(0, 1, 0, 0));

	t.rotationQuat = XMQuaternionRotationMatrix(rotmat);

	t.position = t.position + ship.Velocity;
}

void SpaceshipMovementSystem::update(ECS_Registry &registry, float dt)
{
	ZoneNamed(SpaceshipMovementSystem, true);
	SCOPE_PROFILE("Spaceship Movement System");
	

	BoidReferenceTag & boidref = registry.get<BoidReferenceTag>();

	//120 fps simulation
	dt = 1.0 / 60.0;
	elapsed = dt;

	auto  posview = registry.view<SpaceshipMovementComponent, TransformComponent>(entt::persistent_t{});

	std::for_each(/*std::execution::par_unseq, */posview.begin(), posview.end(), [&](const auto entity) {

		UpdateSpaceship(posview.get<SpaceshipMovementComponent>(entity), posview.get<TransformComponent>(entity), boidref, dt);
		
	});
}

static bool bEven = false;

void SpaceshipMovementSystem::update(ECS_GameWorld & world)
{
	ZoneNamed(SpaceshipMovementSystem, true);
	SCOPE_PROFILE("Spaceship Movement System");

	BoidReferenceTag & boidref = world.registry_entt.get<BoidReferenceTag>();

	ApplicationInfo& appInfo = *world.registry_decs.get_singleton<ApplicationInfo>();
	appInfo.BoidEntities = 0;
	//world.registry_decs.for_each([&](SpaceshipMovementComponent& spaceship, TransformComponent& transform) {
	//	UpdateSpaceship(spaceship, transform, boidref, 1.0 / 30.f);
	//	appInfo.BoidEntities++;
	//});

	Query spaceshipQuery;
	spaceshipQuery.with<SpaceshipMovementComponent, TransformComponent>();
	spaceshipQuery.build();

	static std::vector<DataChunk*> chunk_cache;
	chunk_cache.clear();
	
	{
		ZoneScopedNC("Spaceship Gather Archetypes", tracy::Color::Green);

		world.registry_decs.gather_chunks(spaceshipQuery, chunk_cache);
		
	}
	std::atomic<int> num;
	std::for_each(std::execution::par, chunk_cache.begin(), chunk_cache.end(), [&](DataChunk* chnk) {
		update_ship_chunk(chnk, boidref, num);
	});


}


void update_ship_chunk(DataChunk* chnk, BoidReferenceTag& boidref, std::atomic<int>& count)
{

	ZoneScopedNC("Spaceship Execute Chunks", tracy::Color::Magenta);

	auto sparray = get_chunk_array<SpaceshipMovementComponent>(chnk);
	auto transfarray = get_chunk_array<TransformComponent>(chnk);

	count += chnk->header.last;
	for (int i = chnk->header.last - 1; i >= 0; i--)
	{
		UpdateSpaceship(sparray[i], transfarray[i], boidref, 1.0 / 30.f);
	}
}
