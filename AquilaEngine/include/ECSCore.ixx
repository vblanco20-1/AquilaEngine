#pragma once

export  module ecscore;

import <DirectXMath.h>		 ;
import <DirectXCollision.h>	 ;
import <concurrentqueue.h>	 ;
import <execution>			 ;
//import "robin_hood.h"		 ;
import <vector>;
import <unordered_map>;
export import decsm;
export{


	using namespace decs;

	using namespace DirectX;

	
	

	struct PositionComponent {
		PositionComponent(XMFLOAT3 _Position) : Position(_Position) {};
		PositionComponent() = default;
		XMFLOAT3 Position;
	};

	struct EngineTimeComponent {
		float delta_time{ 0.f };
		uint64_t frameNumber{ 0 };
	};

	struct ExplosionFXComponent {

		float elapsed = 0;
	};
	struct TransformComponent {
		XMVECTOR position;
		XMVECTOR rotationQuat;
		XMVECTOR scale;

		TransformComponent() : rotationQuat(XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0)), scale(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f)) {
		}
	};

	struct StaticTransform {
	};


	class ECS_GameWorldBase {
	public:
		EngineTimeComponent GetTime() {
			return *registry_decs.get_singleton<EngineTimeComponent>();
		}
		ECSWorld registry_decs;
		uint64_t frameNumber;
		template<typename C>
		void mark_components_changed(ComponentArray<C>& clist)
		{
			clist.set_version(frameNumber);
		}
	};
	ECS_GameWorldBase* GameWorld{nullptr};
	struct  System {
		virtual ~System() {}

		virtual void update(ECS_GameWorldBase& world) {};
	};

	struct RotatorComponent {
		XMFLOAT3 Axis;
		float rate;
	};
	struct CullSphere {
		BoundingSphere sphere; //x,y,z lenght
	};
	struct Culled {
		//float c;
	};
	struct CullBitmask {
		uint8_t mask;
	};
	struct IgnoreCull {
		float c;
	};
	struct RenderMatrixComponent {
		RenderMatrixComponent(XMMATRIX&& other)
		{
			Matrix = other;
		}
		RenderMatrixComponent() = default;
		XMMATRIX Matrix;
	};

	struct LocalMatrix {
		LocalMatrix(XMMATRIX&& other)
		{
			Matrix = other;
		}
		LocalMatrix() = default;
		XMMATRIX Matrix;
	};

	struct CubeRendererComponent {
		float randomval = 1.f;
		bool bVisible = true;
		XMFLOAT3 color;

	};
	struct SpaceshipMovementComponent {
		XMFLOAT3 Velocity;
		XMFLOAT3 Heading;
		XMFLOAT3 Target;
		float speed;
	};

	struct CameraComponent {
		XMVECTOR focusPoint;// = XMVectorSet(0, 0, 0, 1);
		XMVECTOR upDirection;// = XMVectorSet(0, 1, 0, 0);
	};
	struct LifetimeComponent {
		float TimeLeft;
	};
	struct SpaceshipSpawnerComponent {

		XMFLOAT3 Bounds;
		float SpawnRate;
		XMVECTOR ShipMoveTarget;
		float Elapsed;
	};

	struct TransformParentComponent {
		decs::CachedRef<RenderMatrixComponent> ParentTransform;
		decs::EntityID Parent;
	};

	template<typename F>
	void parallel_for_chunk(std::vector<DataChunk*>& chunks, F&& functor) {
		std::for_each(/*std::execution::par, */chunks.begin(), chunks.end(), functor);
	}

	template<typename T, typename Traits, typename F>
	void bulk_dequeue(moodycamel::ConcurrentQueue<T, Traits>& queue, F&& fun) {
		T block[Traits::BLOCK_SIZE];
		while (true)
		{
			int dequeued = queue.try_dequeue_bulk(block, Traits::BLOCK_SIZE);
			if (dequeued > 0)
			{
				for (int i = 0; i < dequeued; i++)
				{
					fun(block[i]);
				}
			}
			else
			{
				return;
			}
		}
	};

}