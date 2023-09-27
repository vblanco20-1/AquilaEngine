#pragma once
#include <PrecompiledHeader.h>

namespace ecs::system {


	struct UpdateTransform : public System {


		struct HierarchyUnit {
			RenderMatrixComponent * mat{ nullptr };
			RenderMatrixComponent * parent{ nullptr };
		};
		
		ecs::system::UpdateTransform() {  };
		static void apply_position(TransformComponent & t, const PositionComponent & p)
		{
			t.tf->position = XMLoadFloat3(&p.Position);
		}




		static void build_matrix(const Transform& t, XMMATRIX& matrix)
		{
			const auto ScaleMat = XMMatrixScalingFromVector(t.scale);
			//const auto TranslationMat = XMMatrixTranslationFromVector(t.position);
			//const auto RotMat = XMMatrixRotationQuaternion(t.rotationQuat);
			//
			//matrix = RotMat * (ScaleMat * TranslationMat);

			float zero = 0;
			XMVECTOR zerovec = XMLoadFloat(&zero);

			matrix= XMMatrixAffineTransformation(t.scale, zerovec, t.rotationQuat, t.position);
		}
		static void build_matrix(const TransformComponent& t, RenderMatrixComponent& matrix)
		{
			//const auto ScaleMat = XMMatrixScalingFromVector(t.tf->scale);
			//const auto TranslationMat = XMMatrixTranslationFromVector(t.tf->position);
			//const auto RotMat = XMMatrixRotationQuaternion(t.tf->rotationQuat);
			//
			//matrix.Matrix = RotMat * (ScaleMat *TranslationMat);


			matrix = get_matrix(*t.tf);
		}
		static XMMATRIX get_matrix(const Transform& t)
		{
			
			//const auto ScaleMat = XMMatrixScalingFromVector(t.scale);
			//const auto TranslationMat = XMMatrixTranslationFromVector(t.position);
			//const auto RotMat = XMMatrixRotationQuaternion(t.rotationQuat);
			//
			//return RotMat * (ScaleMat * TranslationMat);

			float zero = 0;
			XMVECTOR zerovec = XMLoadFloat(&zero);

			return XMMatrixAffineTransformation(t.scale,zerovec,t.rotationQuat,t.position);
			
		}


		static void build_matrix(const TransformComponent& t, XMMATRIX& matrix)
		{
			build_matrix(t,matrix);
		}

	
		decs::PureSystemBase* update_root_puresys();

		decs::PureSystemBase* update_hierarchy_puresys();

		void update_root(ECS_GameWorld& world);
		void update_hierarchy(ECS_GameWorld& world, tf::Subflow& sf);
	};
}

inline void refresh_matrix(SceneNode* root) {
	auto& mtx = root->matrix;

	SceneNode* c = root->firstChild;
	if (c) {
		c->matrix = ecs::system::UpdateTransform::get_matrix(*c->transform) * mtx;
		refresh_matrix(c);
		c = c->nextBrother;

		while (c != root->firstChild) {
			c->matrix = ecs::system::UpdateTransform::get_matrix(*c->transform) * mtx;
			refresh_matrix(c);
			c = c->nextBrother;
		}
	}

	//for (auto c : root->children) {
	//	c->matrix = ecs::system::UpdateTransform::get_matrix(*c->transform) * mtx;
	//	refresh_matrix(c);
	//}
}
void update_children_transform(DataChunk* chnk, ECS_GameWorld& world);
void update_root_transform_arrays(DataChunk* chnk, RenderMatrixComponent* __restrict matarray, TransformComponent* __restrict transfarray, PositionComponent* __restrict posarray);
void update_root_transform(DataChunk* chnk);
void update_children_transform_arrays(decs::ECSWorld* world, DataChunk* chnk, RenderMatrixComponent* __restrict matarray, TransformComponent* __restrict transfarray, TransformParentComponent* __restrict parentarray);