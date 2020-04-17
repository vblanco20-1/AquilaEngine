#pragma once
#include <PrecompiledHeader.h>
#include "GameSystems.h"
namespace ecs::system {


	struct UpdateTransform : public System {


		struct HierarchyUnit {
			RenderMatrixComponent * mat{ nullptr };
			RenderMatrixComponent * parent{ nullptr };
		};
		
		ecs::system::UpdateTransform() {  };
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

		virtual void update(ECS_GameWorld& world);

		void update_root(ECS_GameWorld& world);
		void update_hierarchy(ECS_GameWorld& world);
	};
}


void update_children_transform(DataChunk* chnk, ECS_GameWorld& world);
void update_root_transform_arrays(DataChunk* chnk, RenderMatrixComponent* __restrict matarray, TransformComponent* __restrict transfarray, PositionComponent* __restrict posarray);
void update_root_transform(DataChunk* chnk);
void update_children_transform_arrays(decs::ECSWorld* world, DataChunk* chnk, RenderMatrixComponent* __restrict matarray, TransformComponent* __restrict transfarray, TransformParentComponent* __restrict parentarray);