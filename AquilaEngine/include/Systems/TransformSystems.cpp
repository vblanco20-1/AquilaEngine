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
			[&](EntityID entity, auto view) {
			auto[t, p] = view.get<TransformComponent, PositionComponent>(entity);
			apply_position(t, p);
		}
			,
			2048  // execute one task at a time,
			, "Worker: Apply Position"
			);

		auto[Ss, Ts] = parallel_for_ecs(subflow, scaleview,
			[&](EntityID entity, auto view) {
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
			[&](EntityID entity, auto view) {
			EntityParentComponent & parent = view.get<EntityParentComponent>(entity);
			RenderMatrixComponent & matrix = view.get<RenderMatrixComponent>(entity);

			apply_parent_matrix(registry, parent, matrix);
		}
			, 1024, "Worker:Lvl 1 Parent"
			);
		auto[S2, T2] = parallel_for_ecs(subflow, lvl2,
			[&](EntityID entity, auto view) {
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
	SCOPE_PROFILE("TransformUpdate System-decs");

	Archetype FullTrasform;
	FullTrasform.AddComponent<RenderMatrixComponent>();
	FullTrasform.AddComponent<TransformComponent>();

	Archetype PositionOnly;
	PositionOnly.AddComponent<PositionComponent>();

	Archetype TransformWithPosition;
	TransformWithPosition.AddComponent<RenderMatrixComponent>();
	TransformWithPosition.AddComponent<TransformComponent>();
	TransformWithPosition.AddComponent<PositionComponent>();

	//iterate blocks that have position
	world.registry_decs.IterateBlocks(FullTrasform.componentlist, [&](ArchetypeBlock & block) {

		auto matarray = block.GetComponentArray<RenderMatrixComponent>();
		auto transfarray = block.GetComponentArray<TransformComponent>();


		const bool bHasPosition = block.myArch.Match(PositionOnly.componentlist) == 1;

		for (int i = block.last - 1; i >= 0; i--)
		{
			auto &t = transfarray.Get(i);
			if (bHasPosition)
			{
				auto posarray = block.GetComponentArray<PositionComponent>();


				auto &p = posarray.Get(i);
				apply_position(t, p);
			}			

			build_matrix(t, matarray.Get(i));

		}
	}, true);
}
