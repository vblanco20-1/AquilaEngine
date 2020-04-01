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

	ZoneNamed(UpdateTransform_Decs, true);	

	SCOPE_PROFILE("TransformUpdate System-decs");

	Query FullTransform;
	FullTransform.with<RenderMatrixComponent, TransformComponent>();
	FullTransform.exclude<StaticTransform>();
	FullTransform.build();

	Query ChildTransform;
	ChildTransform.with<RenderMatrixComponent, TransformComponent, TransformParentComponent>();
	ChildTransform.exclude<StaticTransform>();
	ChildTransform.build();

	static std::vector<DataChunk*> full_chunk_cache;
	full_chunk_cache.clear();

	static std::vector<DataChunk*> child_chunk_cache;
	child_chunk_cache.clear();



	{
		ZoneScopedNC("Transform Gather Archetypes: ", tracy::Color::Green);

		world.registry_decs.gather_chunks(FullTransform, full_chunk_cache);
		world.registry_decs.gather_chunks(ChildTransform, child_chunk_cache);
	}

	parallel_for_chunk(full_chunk_cache, [&](DataChunk* chnk) {

		update_root_transform(chnk);
	});

	
	parallel_for_chunk(child_chunk_cache, [&](DataChunk* chnk) {
	
		update_children_transform(chnk, world);
	});


}

void update_root_transform(DataChunk* chnk)
{
	ZoneScopedNC("Transform chunk execute: ", tracy::Color::Red);

	auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);
	auto transfarray = get_chunk_array<TransformComponent>(chnk);
	auto posarray = get_chunk_array<PositionComponent>(chnk);

	const bool bHasPosition = posarray.chunkOwner == chnk;

	for (int i = chnk->header.last - 1; i >= 0; i--)
	{
		TransformComponent& t = transfarray[i];
		if (bHasPosition)
		{
			PositionComponent& p = posarray[i];
			ecs::system::UpdateTransform::apply_position(t, p);
		}

		ecs::system::UpdateTransform::build_matrix(t, matarray[i]);
	}	
}

void update_children_transform(DataChunk* chnk, ECS_GameWorld& world)
{
	ZoneScopedNC("Transform chunk execute: children", tracy::Color::Orange);

	auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);
	auto transfarray = get_chunk_array<TransformComponent>(chnk);

	auto parentarray = get_chunk_array<TransformParentComponent>(chnk);

	//unrolled for perf
	int i = 0;
	for (i;i < (chnk->header.last & ~3); i += 4) {


		TransformParentComponent& tpc1 = parentarray[i + 0];
		TransformParentComponent& tpc2 = parentarray[i + 1];
		TransformParentComponent& tpc3 = parentarray[i + 2];
		TransformParentComponent& tpc4 = parentarray[i + 3];


		matarray[i + 0].Matrix *= (tpc1.ParentTransform.get_from(&world.registry_decs, tpc1.Parent))->Matrix;
		matarray[i + 1].Matrix *= (tpc2.ParentTransform.get_from(&world.registry_decs, tpc2.Parent))->Matrix;
		matarray[i + 2].Matrix *= (tpc3.ParentTransform.get_from(&world.registry_decs, tpc3.Parent))->Matrix;
		matarray[i + 3].Matrix *= (tpc4.ParentTransform.get_from(&world.registry_decs, tpc4.Parent))->Matrix;
	}

	for (i; i < (chnk->header.last); i++) {

		TransformParentComponent& tpc = parentarray[i];	

		const XMMATRIX& parent_matrix = (tpc.ParentTransform.get_from(&world.registry_decs, tpc.Parent))->Matrix;

		matarray[i].Matrix *= parent_matrix;
	}

	//for (int i = chnk->header.last - 1; i >= 0; i--)
	//{
	//	TransformComponent& t = transfarray[i];
	//	TransformParentComponent& tpc = parentarray[i];
	//	//const XMMATRIX& parent_matrix = world.registry_decs.get_component<RenderMatrixComponent>(parentarray[i].Parent).Matrix;
	//
	//	const XMMATRIX& parent_matrix = (tpc.ParentTransform.get_from(&world.registry_decs,tpc.Parent)  )->Matrix;
	//
	//	matarray[i].Matrix *= parent_matrix;
	//}
}
