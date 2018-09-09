#pragma once
#include <PrecompiledHeader.h>
struct TransformUpdateSystem : public System {


	struct HierarchyUnit {
		RenderMatrixComponent * mat{ nullptr };
		RenderMatrixComponent * parent{ nullptr };
	};



	struct TransformHierarchy {

		//std::vector< multi_vector<HierarchyUnit,16>> Hierarchy;
		std::vector< moodycamel::ConcurrentQueue<HierarchyUnit>> Queues;
	};
	TransformUpdateSystem() { uses_threading = true; };
	static void ApplyPosition(TransformComponent & t, const PositionComponent & p)
	{
		t.position = XMLoadFloat3(&p.Position);
	}
	static void BuildMatrix(const TransformComponent & t, RenderMatrixComponent & matrix)
	{
		const auto ScaleMat = XMMatrixScalingFromVector(t.scale);
		const auto TranslationMat = XMMatrixTranslationFromVector(t.position);
		const auto RotMat = XMMatrixRotationQuaternion(t.rotationQuat);

		matrix.Matrix = RotMat * (ScaleMat *TranslationMat);
	}
	static void ApplyParentMatrix(ECS_Registry &registry, EntityParentComponent & parent, RenderMatrixComponent & matrix)
	{
		if (parent.Valid(registry) && parent.hierarchyDepth < 5)
		{
			const RenderMatrixComponent & parentmatrix = registry.get<RenderMatrixComponent>(parent.parent);
			matrix.Matrix = matrix.Matrix * parentmatrix.Matrix;
		}
		else
		{

			matrix.Matrix = XMMatrixScaling(0, 0, 0);
			//registry.accommodate<LifetimeComponent>(entity, 0.0f);
		}
	}



	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

		const float dt = get_delta_time(registry);
		ecs::Task task = task_engine.silent_emplace([&, dt](auto& subflow) {

			auto  posview = registry.view<TransformComponent, PositionComponent>(entt::persistent_t{});
			auto  scaleview = registry.view<RenderMatrixComponent, TransformComponent>(entt::persistent_t{});

			auto[Sp, Tp] = parallel_for_ecs(subflow,
				posview,
				[&](EntityID entity, auto view) {
				auto[t, p] = view.get<TransformComponent, PositionComponent>(entity);
				ApplyPosition(t, p);
			}
				,
				2048  // execute one task at a time,
				, "Worker: Apply Position"
				);

			auto[Ss, Ts] = parallel_for_ecs(subflow, scaleview,
				[&](EntityID entity, auto view) {
				auto[matrix, t] = view.get<RenderMatrixComponent, TransformComponent>(entity);
				BuildMatrix(t, matrix);
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

				ApplyParentMatrix(registry, parent, matrix);
			}
				, 1024, "Worker:Lvl 1 Parent"
				);
			auto[S2, T2] = parallel_for_ecs(subflow, lvl2,
				[&](EntityID entity, auto view) {
				EntityParentComponent & parent = view.get<EntityParentComponent>(entity);
				RenderMatrixComponent & matrix = view.get<RenderMatrixComponent>(entity);

				ApplyParentMatrix(registry, parent, matrix);
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
	};

	//struct HierarchyPool {
	//	std::vector<std::pair<RenderMatrixComponent&,EntityID> Entities;
	//};
	//




	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(TransformSystem, 0);
		SCOPE_PROFILE("TransformUpdate System");

		auto  posview = registry.view<TransformComponent, PositionComponent>(entt::persistent_t{});
		auto  scaleview = registry.view<RenderMatrixComponent, TransformComponent>(entt::persistent_t{});
		//auto  rotview = registry.view<RenderMatrixComponent, RotationComponent>(entt::persistent_t{});

		TransformHierarchy Hierarchy;

		{
			rmt_ScopedCPUSample(ApplyPosition, 0);


			std::for_each(/*std::execution::par_unseq,*/ posview.begin(), posview.end(), [&posview](const auto entity) {
				auto[t, p] = posview.get<TransformComponent, PositionComponent>(entity);
				ApplyPosition(t, p);
			});
		}

		{
			rmt_ScopedCPUSample(CalculateMatrices, 0);

			std::for_each(/*std::execution::par, */scaleview.begin(), scaleview.end(), [&scaleview](const auto entity) {

				auto[matrix, t] = scaleview.get<RenderMatrixComponent, TransformComponent>(entity);
				BuildMatrix(t, matrix);

			});
		}
		{
			rmt_ScopedCPUSample(CalculateHierarchy, 0);


			int iterations = 0;
			int invalid = 0;
			int lasthierarchy = 0;
			auto hierarchyview = registry.view<EntityParentComponent, RenderMatrixComponent, Level1Transform>(entt::persistent_t{});

			auto lvl1 = registry.view<EntityParentComponent, RenderMatrixComponent, Level1Transform>(entt::persistent_t{});
			auto lvl2 = registry.view<EntityParentComponent, RenderMatrixComponent, Level2Transform>(entt::persistent_t{});


			std::for_each(/*std::execution::par, */lvl1.begin(), lvl1.end(), [&](const auto entity) {


				EntityParentComponent & parent = lvl1.get<EntityParentComponent>(entity);
				RenderMatrixComponent & matrix = lvl1.get<RenderMatrixComponent>(entity);

				ApplyParentMatrix(registry, parent, matrix);


			});
			std::for_each(/*std::execution::par, */lvl2.begin(), lvl2.end(), [&](const auto entity) {

				EntityParentComponent & parent = lvl2.get<EntityParentComponent>(entity);
				RenderMatrixComponent & matrix = lvl2.get<RenderMatrixComponent>(entity);

				ApplyParentMatrix(registry, parent, matrix);

				//if (parent.Valid(registry) && parent.hierarchyDepth < 5)
				//{
				//	//if (v.size() < 5) { v.reserve(100) };
				//	//HierarchyUnit unit;
				//	//unit.mat = &matrix;
				//	//unit.parent = &registry.get<RenderMatrixComponent>(parent.parent);
				//	//Hierarchy.Hierarchy[parent.hierarchyDepth].get_vector().push_back(unit);
				//	//Hierarchy.Queues[parent.hierarchyDepth].enqueue(unit);
				//	//iterations++;
				//	const RenderMatrixComponent & parentmatrix = registry.get<RenderMatrixComponent>(parent.parent);
				//	matrix.Matrix = matrix.Matrix * parentmatrix.Matrix;
				//}
				//else
				//{
				//	invalid++;
				//	matrix.Matrix = XMMatrixScaling(0, 0, 0);
				//	registry.accommodate<LifetimeComponent>(entity, 0.0f);
				//}
			});
		}

		//std::cout << iterations << invalid;

	};
};