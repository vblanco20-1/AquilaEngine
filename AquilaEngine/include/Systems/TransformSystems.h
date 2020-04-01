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
			
		};

		

	};
}


void update_children_transform(DataChunk* chnk, ECS_GameWorld& world);
void update_root_transform(DataChunk* chnk);