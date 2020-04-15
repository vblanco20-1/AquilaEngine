#pragma once
#include <PrecompiledHeader.h>

#include "decs.h"
using namespace decs;


using namespace DirectX;

class ECS_GameWorld;
extern ECS_GameWorld * GameWorld;

struct PositionComponent{
	PositionComponent(XMFLOAT3 _Position) : Position(_Position) {};
	PositionComponent() = default;
	XMFLOAT3 Position;
};

struct EngineTimeComponent {
	float delta_time;
};

struct ExplosionFXComponent {
	
	float elapsed;
};
struct TransformComponent {
	XMVECTOR position;
	XMVECTOR rotationQuat;
	XMVECTOR scale;	
	
	TransformComponent() : rotationQuat(XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),0)), scale(XMVectorSet(1.0f,1.0f,1.0f,1.0f)){		
	}
};

struct StaticTransform {
};

namespace ecs {
	using TaskEngine = tf::Taskflow;//tf::BasicTaskflow<std::function<void()>>;
	using Task = typename TaskEngine::TaskType;
	using SubflowBuilder = typename TaskEngine::SubflowBuilderType;
}


struct  System {
	virtual ~System() {}	

	virtual void update(ECS_GameWorld & world);
};

struct RotatorComponent {
	XMVECTOR Axis;
	float rate;
};

struct Culled {
	//float c;
};
struct IgnoreCull {
	float c;
};
struct RenderMatrixComponent {
	RenderMatrixComponent(XMMATRIX && other)
	{
		Matrix = other;
	}
	RenderMatrixComponent() {

	}
	XMMATRIX Matrix;
};
struct CubeRendererComponent{
	float randomval;
	bool bVisible;
	XMFLOAT3 color;
	CubeRendererComponent() {
	
	}
	//XMMATRIX Matrix;
};
struct SpaceshipMovementComponent {
	XMVECTOR Velocity;
	XMVECTOR Heading;
	XMVECTOR Target;
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
	std::for_each(std::execution::par, chunks.begin(), chunks.end(), functor);
}

template <typename C, typename V>
auto parallel_for_ecs(ecs::SubflowBuilder &builder, V& view, /*std::function<void(EntityID, V&)>*/C&& c, size_t g, const char*profile_name = "par_for") {

	auto beg = view.begin();
	auto end = view.end();

	auto source = builder.placeholder();
	auto target = builder.placeholder();

	V *mview = new V(std::move(view));

	while (beg != end) {

		auto e = beg;
		
		size_t r = std::distance(beg, end);
		std::advance(e, std::min(r, g));
		int namelen = strlen(profile_name);
		// Create a task
		auto task = builder.silent_emplace([mview,profile_name, beg, e, c, namelen]() mutable {
			//rmt_BeginCPUSampleDynamic(profile_name, 0);
			ZoneScoped;
			ZoneText(profile_name, namelen);

			for (auto it = beg; it != e; it++)
			{
				c(*it,*mview);
			}
			//std::for_each(beg, e, c);
			//rmt_EndCPUSample();
		});
		source.precede(task);
		task.precede(target);
		
		beg = e;
	}

	return std::make_pair(source, target);
}