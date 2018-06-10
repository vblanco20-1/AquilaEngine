#pragma once
//#include <PrecompiledHeader.h>
#include <stdint.h>
#include "entt/entt.hpp"



#include <DirectXMath.h>

using namespace DirectX;
using EntityID = std::uint64_t;
using EntityVersion = entt::Registry<std::uint64_t>::version_type;
using ECS_Registry = entt::Registry<std::uint64_t>;


class ECS_GameWorld;
extern ECS_GameWorld * GameWorld;

struct PositionComponent {
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



struct  System {

	virtual ~System() {}
	virtual void initialize(ECS_Registry &registry) {

	};
	virtual void update(ECS_Registry &registry, float dt) = 0;

	virtual void cleanup(ECS_Registry &registry) {

	};

};

struct RotatorComponent {
	XMVECTOR Axis;
	float rate;
};

struct RenderMatrixComponent {
	XMMATRIX Matrix;
};
struct CubeRendererComponent {
	float randomval;
	bool bVisible;
	XMFLOAT3 color;
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

