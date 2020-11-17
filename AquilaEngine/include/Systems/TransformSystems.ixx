export module transformsystems;

import ecscore;
import decsm;

import <DirectXMath.h>;
import <DirectXCollision.h>;
import <vector>;
import <unordered_map>;

using namespace decs;
using namespace DirectX;

export{
	namespace ecs::system {

		struct UpdateTransform : public System {
			

			struct HierarchyUnit {
				RenderMatrixComponent* mat{ nullptr };
				RenderMatrixComponent* parent{ nullptr };
			};

			ecs::system::UpdateTransform() {  };
			static void apply_position(TransformComponent& t, const PositionComponent& p)
			{
				t.position = XMLoadFloat3(&p.Position);
			}


			static void build_matrix(const TransformComponent& t, RenderMatrixComponent& matrix)
			{
				const auto ScaleMat = DirectX::XMMatrixScalingFromVector(t.scale);
				const auto TranslationMat = DirectX::XMMatrixTranslationFromVector(t.position);
				const auto RotMat = DirectX::XMMatrixRotationQuaternion(t.rotationQuat);

				matrix.Matrix = RotMat * (ScaleMat * TranslationMat);
			}

			static void build_matrix(const TransformComponent& t, DirectX::XMMATRIX& matrix)
			{
				const auto ScaleMat = DirectX::XMMatrixScalingFromVector(t.scale);
				const auto TranslationMat = DirectX::XMMatrixTranslationFromVector(t.position);
				const auto RotMat = DirectX::XMMatrixRotationQuaternion(t.rotationQuat);

				matrix = RotMat * (ScaleMat * TranslationMat);
			}

			void update_root(ECS_GameWorldBase& world);
			void update_hierarchy(ECS_GameWorldBase& world);
		};
	}


	void update_children_transform(DataChunk* chnk, ECS_GameWorldBase& world);
	void update_root_transform_arrays(DataChunk* chnk, RenderMatrixComponent* __restrict matarray, TransformComponent* __restrict transfarray, PositionComponent* __restrict posarray);
	void update_root_transform(DataChunk* chnk);
	void update_children_transform_arrays(ECSWorld* world, DataChunk* chnk, RenderMatrixComponent* __restrict matarray, TransformComponent* __restrict transfarray, TransformParentComponent* __restrict parentarray);
}