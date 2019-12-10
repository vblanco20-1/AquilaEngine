#pragma once
#include <PrecompiledHeader.h>
#include "GameSystems.h"
namespace ecs::system {


	struct UpdateTransform : public System {


		struct HierarchyUnit {
			RenderMatrixComponent * mat{ nullptr };
			RenderMatrixComponent * parent{ nullptr };
		};
		
		ecs::system::UpdateTransform() { uses_threading = true; };
		static void apply_position(TransformComponent & t, const PositionComponent & p)
		{
			t.position = XMLoadFloat3(&p.Position);
		}


		static void build_matrix(const TransformComponent & t, RenderMatrixComponent & matrix)
		{
			const auto ScaleMat = XMMatrixScalingFromVector(t.scale);
			const auto TranslationMat = XMMatrixTranslationFromVector(t.position);
			const auto RotMat = XMMatrixRotationQuaternion(t.rotationQuat);

			matrix.Matrix = RotMat * (ScaleMat *TranslationMat);
		}


		static void apply_parent_matrix(ECS_Registry &registry, EntityParentComponent & parent, RenderMatrixComponent & matrix)
		{
			//if (parent.Valid(registry) && parent.hierarchyDepth < 5)
			//{
			//	const RenderMatrixComponent & parentmatrix = registry.get<RenderMatrixComponent>(parent.parent);
			//	matrix.Matrix = matrix.Matrix * parentmatrix.Matrix;
			//}
			//else
			//{
			//	matrix.Matrix = XMMatrixScaling(0, 0, 0);				
			//}
		}


		virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent);;

		virtual void update(ECS_GameWorld & world);
		virtual void update(ECS_Registry &registry, float dt)
		{
			ZoneNamed(TransformSystem, true);
			SCOPE_PROFILE("TransformUpdate System");

			auto  posview = registry.view<TransformComponent, PositionComponent>(entt::persistent_t{});
			auto  matview = registry.view<RenderMatrixComponent, TransformComponent>(entt::persistent_t{});
		
			
			{
				ZoneNamed(TransformApplyPosition, true);


				std::for_each(/*std::execution::par_unseq,*/ posview.begin(), posview.end(), [&posview](const auto entity) {
					auto[t, p] = posview.get<TransformComponent, PositionComponent>(entity);
					apply_position(t, p);
				});
			}

			{
				ZoneNamed(CalculateMatrices, true);

				std::for_each(/*std::execution::par, */matview.begin(), matview.end(), [&matview](const auto entity) {

					auto[matrix, t] = matview.get<RenderMatrixComponent, TransformComponent>(entity);
					build_matrix(t, matrix);
				});
			}
			{
				ZoneNamed(CalculateHierarchy, true);


				int iterations = 0;
				int invalid = 0;
				int lasthierarchy = 0;
				auto hierarchyview = registry.view<EntityParentComponent, RenderMatrixComponent, Level1Transform>(entt::persistent_t{});

				auto lvl1 = registry.view<EntityParentComponent, RenderMatrixComponent, Level1Transform>(entt::persistent_t{});
				auto lvl2 = registry.view<EntityParentComponent, RenderMatrixComponent, Level2Transform>(entt::persistent_t{});


				std::for_each(/*std::execution::par, */lvl1.begin(), lvl1.end(), [&](const auto entity) {


					EntityParentComponent & parent = lvl1.get<EntityParentComponent>(entity);
					RenderMatrixComponent & matrix = lvl1.get<RenderMatrixComponent>(entity);

					apply_parent_matrix(registry, parent, matrix);

				});
				std::for_each(/*std::execution::par, */lvl2.begin(), lvl2.end(), [&](const auto entity) {

					EntityParentComponent & parent = lvl2.get<EntityParentComponent>(entity);
					RenderMatrixComponent & matrix = lvl2.get<RenderMatrixComponent>(entity);

					apply_parent_matrix(registry, parent, matrix);
										
				});
			}
		};
	};
}