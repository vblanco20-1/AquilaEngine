#pragma once
#include <PrecompiledHeader.h>

#include "decs/decs.hpp"
//#include <stdint.h>
//#include "entt/entt.hpp"

//#include "taskflow/taskflow.hpp"

//#include <DirectXMath.h>

using namespace DirectX;
using EntityID = std::uint64_t;
using EntityVersion = entt::Registry<std::uint64_t>::version_type;
using ECS_Registry = entt::Registry<std::uint64_t>;


class ECS_GameWorld;
extern ECS_GameWorld * GameWorld;

struct PositionComponent{
	PositionComponent(XMFLOAT3 _Position) : Position(_Position) {};
	PositionComponent() = default;
	XMFLOAT3 Position;
};
//struct ScaleComponent {
//	XMVECTOR Scale3D;
//
//};
//struct RotationComponent {
//	XMVECTOR RotationAxis;
//	float Angle;
//};


//struct HierarchyNode {
//	XMMATRIX Transform;
//	uint16_t Parent;
//};
//
//struct NodeTree {
//
//	std::vector< HierarchyNode > Levels;
//};

//NodeTree & GetTransformTree(ECS_Registry & reg)
//{
//	if (reg.has<NodeTree>())
//	{
//		return reg.get<NodeTree>();
//	}
//	else
//	{
//		return reg.assign<NodeTree>();
//	}
//}

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
struct EntityParentComponent {

	//depth zero for a world transform
	uint8_t hierarchyDepth;
	EntityID parent;
	EntityVersion entityVersion;
	//EntityParentComponent() :parent(0), hierarchyDepth(0) {}
	
	bool Valid(ECS_Registry & registry)
	{
		return registry.current(parent) == registry.version(parent);
	}

	void SetParent(EntityID newParent, ECS_Registry & registry)
	{
		parent = newParent;
		hierarchyDepth = 1;
		//entityVersion = registry.current(newParent);
		if (registry.has<EntityParentComponent>(newParent))
		{
			hierarchyDepth = registry.get<EntityParentComponent>(newParent).hierarchyDepth + 1;
		}
	}
};


namespace ecs {
	using TaskEngine = tf::Taskflow;//tf::BasicTaskflow<std::function<void()>>;
	using Task = typename TaskEngine::TaskType;
	using SubflowBuilder = typename TaskEngine::SubflowBuilderType;
}

struct RendererRegistryReferenceComponent {

	ECS_Registry * rg;
};

struct  System {

	bool uses_threading{ false };

	virtual ~System() {}

	float get_delta_time(ECS_Registry &registry) {
		return registry.get<EngineTimeComponent>().delta_time;

	}
	

	virtual ecs::Task schedule(ECS_Registry &registry,ecs::TaskEngine & task_engine , ecs::Task & parent, ecs::Task & grandparent) { 
	
		ecs::Task task = task_engine.placeholder();
		task.name("Base System Schedule");
		//run after the parent
		task.gather(parent);
		return std::move(task);
	};

	virtual void initialize(ECS_Registry &registry) {};
	virtual void update(ECS_Registry &registry, float dt) = 0;
	virtual void update(ECS_GameWorld & world) {};

	virtual void cleanup(ECS_Registry &registry) {};

};

struct RotatorComponent {
	XMVECTOR Axis;
	float rate;
};

struct Culled {

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
		
		// Create a task
		auto task = builder.silent_emplace([mview,profile_name, beg, e, c]() mutable {
			rmt_BeginCPUSampleDynamic(profile_name, 0);


			for (auto it = beg; it != e; it++)
			{
				c(*it,*mview);
			}
			//std::for_each(beg, e, c);
			rmt_EndCPUSample();
		});
		source.precede(task);
		task.precede(target);
		
		beg = e;
	}

	return std::make_pair(source, target);
}