#include "Systems/SpaceshipSystems.h"
#include "Systems/BoidSystems.h"
#include "SimpleProfiler.h"
#include "taskflow/taskflow.hpp"

#include "GameWorld.h"
#include "ApplicationInfoUI.h"

void UpdateSpaceship(SpaceshipMovementComponent & SpaceshipMov, TransformComponent & Transform, BoidReferenceTag & boidref, float DeltaTime);

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


static bool bEven = false;

void SpaceshipMovementSystem::update(ECS_GameWorld & world)
{
	ZoneNamed(SpaceshipMovementSystem, true);
	SCOPE_PROFILE("Spaceship Movement System");

	BoidReferenceTag & boidref = *world.registry_decs.get_singleton<BoidReferenceTag>();

	ApplicationInfo& appInfo = *world.registry_decs.get_singleton<ApplicationInfo>();
	appInfo.BoidEntities = 0;


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
