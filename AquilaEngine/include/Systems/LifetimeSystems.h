#pragma once

#include <PrecompiledHeader.h>

void BuildExplosionEffect(const XMVECTOR Position, ECS_Registry & registry)
{
	auto explosion = registry.create();
	registry.assign<CubeRendererComponent>(explosion);
	registry.get<CubeRendererComponent>(explosion).color = XMFLOAT3(0.0, 1.0, 1.0);
	registry.assign<TransformComponent>(explosion);
	registry.get<TransformComponent>(explosion).position = Position;
	//registry.get<TransformComponent>(explosion) = registry.get<TransformComponent>(e);
	registry.assign<RenderMatrixComponent>(explosion, XMMATRIX{});
	//
	registry.assign<LifetimeComponent>(explosion, 2.f / 5.0f);
	registry.assign < ExplosionFXComponent>(explosion);

}
struct DestructionSystem : public System {

	struct ExplosionSpawnStruct {
		XMFLOAT3 position;
	};
	struct QueueTraits : public moodycamel::ConcurrentQueueDefaultTraits
	{
		static const size_t BLOCK_SIZE = 256;		// Use bigger blocks
	};


	moodycamel::ConcurrentQueue<EntityID, QueueTraits> EntitiesToDelete;
	moodycamel::ConcurrentQueue<ExplosionSpawnStruct, QueueTraits> ExplosionsToSpawn;
	DestructionSystem() { uses_threading = true; };
	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

		ecs::Task synctask = grandparent;//task_engine.placeholder();
										 //synctask.precede(parent);
										 //synctask.name("destruction sync");

		const float dt = get_delta_time(registry);
		ecs::Task lifetask = task_engine.silent_emplace([&, dt]() {
			CountLifetimes(registry, dt);
			//update(registry, dt);
		});
		lifetask.name("Lifetime System");

		synctask.precede(lifetask);


		ecs::Task applytask = task_engine.silent_emplace([&, dt]() {
			ApplyDestruction(registry, dt);
			//update(registry, dt);
		});
		applytask.name("ApplyDestruction System");


		//run after the parent
		applytask.gather(parent, lifetask);
		return std::move(applytask);
	};
	void CountLifetimes(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(Destruction_Iterate, 0);
		auto  view = registry.view<LifetimeComponent>(/*entt::persistent_t{}*/);



		std::for_each(/*std::execution::par,*/ view.begin(), view.end(), [&](const auto e) {

			auto &life = view.get(e);

			//view.each([&, dt](auto & e, LifetimeComponent & life) {
			life.TimeLeft -= dt;
			if (life.TimeLeft < 0)
			{
				if (registry.has<SpaceshipMovementComponent>(e))
				{
					//BuildExplosionEffect(registry.get<TransformComponent>(e).position,   registry);
					//was a spaceship, create explosion fx;

					auto & p = registry.get<TransformComponent>(e).position;

					ExplosionSpawnStruct ex;

					ex.position.x = XMVectorGetX(p);
					ex.position.y = XMVectorGetY(p);
					ex.position.z = XMVectorGetZ(p);
					ExplosionsToSpawn.enqueue(ex);


				}
				//registry.destroy(e);
				EntitiesToDelete.enqueue(e);
			}
		});

	}

	void ApplyDestruction(ECS_Registry & registry, float dt)
	{
		rmt_ScopedCPUSample(Destruction_Apply, 0);

		EntityID EntitiesToDestroy[64];
		while (true)
		{
			int dequeued = EntitiesToDelete.try_dequeue_bulk(EntitiesToDestroy, 64);
			if (dequeued > 0)
			{

				for (int i = 0; i < dequeued; i++)
				{
					registry.destroy(EntitiesToDestroy[i]);
				}
			}
			else
			{
				break;
			}
		}

		ExplosionSpawnStruct ex[64];
		while (true)
		{
			int dequeued = ExplosionsToSpawn.try_dequeue_bulk(ex, 64);
			if (dequeued > 0)
			{

				for (int i = 0; i < dequeued; i++)
				{
					BuildExplosionEffect(XMLoadFloat3(&ex[i].position), registry);
					//registry.destroy(EntitiesToDestroy[i]);
				}
			}
			else
			{
				break;
			}
		}
	}

	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(DestructionSystem, 0);

		CountLifetimes(registry, dt);
		ApplyDestruction(registry, dt);
	}
};

struct ExplosionFXSystem : public System {

	ExplosionFXSystem() { uses_threading = true; };

	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

		const float dt = get_delta_time(registry);
		ecs::Task task = task_engine.silent_emplace([&, dt]() {

			update(registry, dt);
		});
		task.name("ExplosionFX System");
		//run after the parent
		task.gather(parent);
		return std::move(task);
	};


	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(ExplosionFXSystem, 0);
		auto  view = registry.view<ExplosionFXComponent, TransformComponent>(entt::persistent_t{});

		std::for_each(/*std::execution::par_unseq, */view.begin(), view.end(), [&](const auto entity) {


			auto[fx, tfc] = view.get<ExplosionFXComponent, TransformComponent>(entity);


			fx.elapsed += 5.0*dt;

			const float sc = 3 + ((1 - abs(fx.elapsed - 1.0f)) / 1.0f) * 5;
			tfc.scale = XMVectorSet(sc, sc, sc, sc);
			//if (life.TimeLeft < 0)
			//{
			//	if (registry.has<SpaceshipMovementComponent>(e))
			//	{
			//		BuildExplosionEffect(registry.get<TransformComponent>(e).position, registry);
			//		//was a spaceship, create explosion fx;
			//
			//
			//
			//	}
			//
			//	registry.destroy(e);
			//}
		});
	}
};

struct RandomFlusherSystem : public System {

	RandomFlusherSystem() { uses_threading = true; };

	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

		//ecs::Task task = task_engine.placeholder();

		const float dt = get_delta_time(registry);
		ecs::Task task = task_engine.silent_emplace([&, dt]() {

			update(registry, dt);
		});

		task.name("Random Flusher System");
		//run after the parent
		task.gather(parent);
		return std::move(task);
	};


	virtual void update(ECS_Registry &registry, float dt)
	{
		return;
		//auto  posview = registry.view<CubeRendererComponent>(/*entt::persistent_t{}*/);
		//posview.each([&, dt](auto & e,CubeRendererComponent & cube) {			
		//	
		//	cube.randomval += 0.1f / 10.0f;			
		//});
	}
};