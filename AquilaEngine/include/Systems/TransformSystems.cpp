#include "TransformSystems.h"
#include "GameWorld.h"


ecs::Task ecs::system::UpdateTransform::schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent)
{
	const float dt = get_delta_time(registry);
	ecs::Task task = task_engine.silent_emplace([&, dt](auto& subflow) {

		auto  posview = registry.view<TransformComponent, PositionComponent>(entt::persistent_t{});
		auto  scaleview = registry.view<RenderMatrixComponent, TransformComponent>(entt::persistent_t{});

		auto[Sp, Tp] = parallel_for_ecs(subflow,
			posview,
			[&](EntityID_entt entity, auto view) {
			auto[t, p] = view.get<TransformComponent, PositionComponent>(entity);
			apply_position(t, p);
		}
			,
			2048  // execute one task at a time,
			, "Worker: Apply Position"
			);

		auto[Ss, Ts] = parallel_for_ecs(subflow, scaleview,
			[&](EntityID_entt entity, auto view) {
			auto[matrix, t] = view.get<RenderMatrixComponent, TransformComponent>(entity);
			build_matrix(t, matrix);
		}
			, 2048, "Worker: Apply Transform"
			);

		int iterations = 0;
		int invalid = 0;
		int lasthierarchy = 0;
		auto hierarchyview = registry.view<EntityParentComponent, RenderMatrixComponent, Level1Transform>(entt::persistent_t{});

		auto lvl1 = registry.view<EntityParentComponent, RenderMatrixComponent, Level1Transform>(entt::persistent_t{});
		auto lvl2 = registry.view<EntityParentComponent, RenderMatrixComponent, Level2Transform>(entt::persistent_t{});

		auto[S1, T1] = parallel_for_ecs(subflow, lvl1,
			[&](EntityID_entt entity, auto view) {
			EntityParentComponent & parent = view.get<EntityParentComponent>(entity);
			RenderMatrixComponent & matrix = view.get<RenderMatrixComponent>(entity);

			apply_parent_matrix(registry, parent, matrix);
		}
			, 1024, "Worker:Lvl 1 Parent"
			);
		auto[S2, T2] = parallel_for_ecs(subflow, lvl2,
			[&](EntityID_entt entity, auto view) {
			EntityParentComponent & parent = view.get<EntityParentComponent>(entity);
			RenderMatrixComponent & matrix = view.get<RenderMatrixComponent>(entity);

			apply_parent_matrix(registry, parent, matrix);
		}
			, 1024, "Worker:Lvl 2 Parent"
			);

		Tp.precede(Ss);
		Ts.precede(S1);
		T1.precede(S2);
		//subflow.linearize(Tp,Ts,T1,T2);

		//update(registry, dt);
	});
	task.name("Transform Update System");
	//run after the parent
	task.gather(parent);
	return std::move(task);
}

void ecs::system::UpdateTransform::update(ECS_GameWorld & world)
{
	update(world.registry_entt, world.GetTime().delta_time);

	rmt_ScopedCPUSample(UpdateTransform_Decs, 0);	

	SCOPE_PROFILE("TransformUpdate System-decs");

	Query FullTransform;
	FullTransform.With<RenderMatrixComponent, TransformComponent>();
	FullTransform.Exclude<StaticTransform>();
	FullTransform.Build();


	static std::vector<DataChunk*> chunk_cache;
	chunk_cache.clear();

	iterate_matching_archetypes(&world.registry_decs, FullTransform, [&](Archetype* arch) {

		for (auto chnk : arch->chunks) {
			chunk_cache.push_back(chnk);
		}
	});

	std::for_each(std::execution::par, chunk_cache.begin(), chunk_cache.end(), [&](DataChunk* chnk) {
			auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);
			auto transfarray = get_chunk_array<TransformComponent>(chnk);
			auto posarray = get_chunk_array<PositionComponent>(chnk);
			
			const bool bHasPosition = posarray.chunkOwner == chnk;
			
			for (int i = chnk->header.last - 1; i >= 0; i--)
			{
				auto& t = transfarray[i];
				if (bHasPosition)
				{
					auto& p = posarray[i];
					apply_position(t, p);
				}

				build_matrix(t, matarray[i]);
			}			
	});


}
