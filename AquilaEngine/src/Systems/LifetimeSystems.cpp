#include <PrecompiledHeader.h>

#include "Systems/LifetimeSystems.h"
#include "GameWorld.h"
#include "Systems/RenderSystems.h"

void BuildExplosionEffect(const XMFLOAT3 Position, ECS_GameWorld& world)
{
	auto reg = &world.registry_decs;
	auto explosion = world.registry_decs.new_entity<CubeRendererComponent, 
		TransformComponent, 
		RenderMatrixComponent, 
		LifetimeComponent, 
		ExplosionFXComponent, CullSphere>();

	reg->get_component<CubeRendererComponent>(explosion).color = XMFLOAT3(0.0, 1.0, 1.0);
	reg->get_component<TransformComponent>(explosion).position = XMVectorSet(Position.x, Position.y, Position.z, 1.f);
	reg->get_component<LifetimeComponent>(explosion).TimeLeft = 2.f / 5.0f;

	CullSphere& sphere = reg->get_component<CullSphere>(explosion);

	ecs::system::FrustrumCuller::update_cull_sphere(&sphere, &reg->get_component<TransformComponent>(explosion), 1.f);
}

void DestructionSystem::update(ECS_GameWorld& world)
{
	SCOPE_PROFILE("Deleter System ")

	ZoneNamed(DestructionSystem, true);


	const float dt = world.GetTime().delta_time;

	auto* reg = &world.registry_decs;


	static std::vector<DataChunk*> chunk_cache;
	chunk_cache.clear();

	Query query;
	query.with<LifetimeComponent>();
	query.build();

	{
		reg->gather_chunks(query, chunk_cache);

		ZoneScopedN("Collect deleteables");
		parallel_for_chunk(chunk_cache, [&](DataChunk* chnk) {
			auto lfarray = get_chunk_array<LifetimeComponent>(chnk);
			auto idarray = get_chunk_array<EntityID>(chnk);
			auto tfarray = get_chunk_array<TransformComponent>(chnk);
			const bool is_spaceship = get_chunk_array<SpaceshipMovementComponent>(chnk).valid() && tfarray.valid();

			for (int i = chnk->header.last - 1; i >= 0; i--)
			{
				lfarray[i].TimeLeft -= dt;
				if (lfarray[i].TimeLeft < 0)
				{
					if (is_spaceship) {
						auto & p = tfarray[i].position;

						ExplosionSpawnStruct ex;

						ex.position.x = XMVectorGetX(p);
						ex.position.y = XMVectorGetY(p);
						ex.position.z = XMVectorGetZ(p);
						ExplosionsToSpawn.enqueue(ex);
					}

					EntitiesToDelete_decs.enqueue(idarray[i]);
				}
			}
		});
	}

	{
		ZoneScopedN("Apply deletions");

		bulk_dequeue(EntitiesToDelete_decs, [&](EntityID& e) {
			reg->destroy(e);
		});
	}
	{
		ZoneScopedN("Make explosions");

		bulk_dequeue(ExplosionsToSpawn, [&](ExplosionSpawnStruct& ex) {
			BuildExplosionEffect(ex.position, world);
		});
	}

	Query query2;
	query2.with<TransformParentComponent>();
	query2.build();

	chunk_cache.clear();
	{
		ZoneScopedN("Collect deleteables");
		reg->gather_chunks(query2, chunk_cache);
		parallel_for_chunk(chunk_cache, [&](DataChunk* chnk) {
			auto lfarray = get_chunk_array<TransformParentComponent>(chnk);
			auto idarray = get_chunk_array<EntityID>(chnk);
			for (int i = chnk->header.last - 1; i >= 0; i--)
			{
				if (!decs::adv::is_entity_valid(reg, lfarray[i].Parent))
				{
					EntitiesToDelete_decs.enqueue(idarray[i]);
				}
			}
		});
	}
	{
		ZoneScopedN("Apply deletions");

		bulk_dequeue(EntitiesToDelete_decs, [&](EntityID& e) {
			reg->destroy(e);
		});
	}
}

void ExplosionFXSystem::update(ECS_GameWorld& world)
{
	ZoneScopedN("ExplosionFXSystem");

	float dt = world.GetTime().delta_time;

	world.registry_decs.for_each([&](ExplosionFXComponent& fx, TransformComponent& transform) {
		
		fx.elapsed += 5.0 * dt;

		const float sc = 3 + ((1 - abs(fx.elapsed - 1.0f)) / 1.0f) * 5;
		transform.scale = XMVectorSet(sc, sc, sc, sc);
	});	
}
