#include <PrecompiledHeader.h>

#include "Systems/SpaceshipSystems.h"
#include "Systems/BoidSystems.h"
#include "SimpleProfiler.h"
#include "GameWorld.h"
#include "ApplicationInfoUI.h"

void UpdateSpaceship(SpaceshipMovementComponent & SpaceshipMov, TransformComponent & Transform, SceneNodeComponent& sceneNode ,BoidMap* boidMap, float DeltaTime)
{
	auto & ship = SpaceshipMov;
	auto & t = Transform; 
	const float dt = DeltaTime;

	XMVECTOR Mov = XMLoadFloat3(&ship.Target) - t.tf->position;
	Mov = XMVector3Normalize(Mov);
	Mov = Mov * ship.speed * dt;

	XMVECTOR ShipVel = XMLoadFloat3(&ship.Velocity) + Mov;
	

	XMVECTOR OffsetVelocity{ 0.0f,0.0f,0.0f,0.0f };
	int num = 10;
	boidMap->Foreach_EntitiesInRadius_Morton(3, t.tf->position, [&](const GridItem2& boid) {
	
		
			XMVECTOR Avoidance = t.tf->position - boid.pos;
			float dist = XMVectorGetX(XMVector3Length(Avoidance));
			OffsetVelocity += XMVector3Normalize(Avoidance) * (1.0f - (std::clamp(dist / 10.0f, 0.0f, 1.0f)));
			num--;
			return num > 0;
	});

	OffsetVelocity *= 10;

	ShipVel = XMVector3ClampLength(ShipVel + OffsetVelocity, 0.0f, ship.speed);

	XMMATRIX rotmat = XMMatrixLookAtLH(t.tf->position, t.tf->position + ShipVel * 10, XMVectorSet(0, 1, 0, 0));

	t.tf->rotationQuat = XMQuaternionRotationMatrix(rotmat);

	t.tf->position = t.tf->position + ShipVel;

	XMStoreFloat3(&ship.Velocity, ShipVel);

	sceneNode.node->matrix = ecs::system::UpdateTransform::get_matrix(*t.tf);
	
	refresh_matrix(sceneNode.node);
}


void update_ship_chunk(DataChunk* chnk, BoidMap* boidMap, std::atomic<int>& count)
{

	ZoneScopedNC("Spaceship Execute Chunks", tracy::Color::Magenta);

	auto sparray = get_chunk_array<SpaceshipMovementComponent>(chnk);
	auto transfarray = get_chunk_array<TransformComponent>(chnk);
	auto nodearray = get_chunk_array<SceneNodeComponent>(chnk);

	count += chnk->count();
	for (int i = chnk->count() - 1; i >= 0; i--)
	{
		UpdateSpaceship(sparray[i], transfarray[i], nodearray[i], boidMap, 1.0 / 30.f);
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

decs::PureSystemBase* SpaceshipMovementSystem::getAsPureSystem()
{
	static auto puresys = [](){
	
		Query spaceshipQuery;
		spaceshipQuery.with<SpaceshipMovementComponent, TransformComponent>();
		spaceshipQuery.build();

		return decs::make_pure_system_chunk(spaceshipQuery,[](void* context,DataChunk* chnk) {

			ZoneScopedN("SpaceshipMovementSystem - pure");

			ECS_GameWorld& world = *reinterpret_cast<ECS_GameWorld*>(context);
			

			BoidReferenceTag* boidref = world.registry_decs.get_singleton<BoidReferenceTag>();
			ApplicationInfo* appInfo = world.registry_decs.get_singleton<ApplicationInfo>();

			std::atomic<int> num;
			update_ship_chunk(chnk, boidref->map, num);

			auto transfarray = get_chunk_array<TransformComponent>(chnk);
			world.mark_components_changed(transfarray);
			
			appInfo->BoidEntities+= num.load();
		});
	}();

	return &puresys;
}

