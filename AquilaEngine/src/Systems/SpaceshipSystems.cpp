#include <PrecompiledHeader.h>

#include "Systems/SpaceshipSystems.h"
#include "Systems/BoidSystems.h"
#include "SimpleProfiler.h"
#include "GameWorld.h"
#include "ApplicationInfoUI.h"

void UpdateSpaceship(SpaceshipMovementComponent & SpaceshipMov, TransformComponent & Transform, BoidMap* boidMap, float DeltaTime)
{
	auto & ship = SpaceshipMov;
	auto & t = Transform; 
	const float dt = DeltaTime;

	XMVECTOR Mov = XMLoadFloat3(&ship.Target) - t.position;
	Mov = XMVector3Normalize(Mov);
	Mov = Mov * ship.speed * dt;

	XMVECTOR ShipVel = XMLoadFloat3(&ship.Velocity) + Mov;
	

	XMVECTOR OffsetVelocity{ 0.0f,0.0f,0.0f,0.0f };
	int num = 10;
	boidMap->Foreach_EntitiesInRadius_Morton(3, t.position, [&](const GridItem2& boid) {
	
		
			XMVECTOR Avoidance = t.position - boid.pos;
			float dist = XMVectorGetX(XMVector3Length(Avoidance));
			OffsetVelocity += XMVector3Normalize(Avoidance) * (1.0f - (std::clamp(dist / 10.0f, 0.0f, 1.0f)));
			num--;
			return num > 0;
	});

	OffsetVelocity *= 10;

	ShipVel = XMVector3ClampLength(ShipVel + OffsetVelocity, 0.0f, ship.speed);

	XMMATRIX rotmat = XMMatrixLookAtLH(t.position, t.position + ShipVel * 10, XMVectorSet(0, 1, 0, 0));

	t.rotationQuat = XMQuaternionRotationMatrix(rotmat);

	t.position = t.position + ShipVel;

	XMStoreFloat3(&ship.Velocity, ShipVel);
}


void update_ship_chunk(DataChunk* chnk, BoidMap* boidMap, std::atomic<int>& count)
{

	ZoneScopedNC("Spaceship Execute Chunks", tracy::Color::Magenta);

	auto sparray = get_chunk_array<SpaceshipMovementComponent>(chnk);
	auto transfarray = get_chunk_array<TransformComponent>(chnk);

	count += chnk->header.last;
	for (int i = chnk->header.last - 1; i >= 0; i--)
	{
		UpdateSpaceship(sparray[i], transfarray[i], boidMap, 1.0 / 30.f);
	}

	
}


void SpaceshipMovementSystem::update(ECS_GameWorld & world)
{
	ZoneNamed(SpaceshipMovementSystem, true);
	SCOPE_PROFILE("Spaceship Movement System");

	BoidReferenceTag* boidref = world.registry_decs.get_singleton<BoidReferenceTag>();

	ApplicationInfo* appInfo = world.registry_decs.get_singleton<ApplicationInfo>();
	appInfo->BoidEntities = 0;


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
	parallel_for_chunk(chunk_cache,[&](DataChunk* chnk) {
		update_ship_chunk(chnk, boidref->map, num);

		auto transfarray = get_chunk_array<TransformComponent>(chnk);
		world.mark_components_changed(transfarray);
	});
	appInfo->BoidEntities = num.load();
}

