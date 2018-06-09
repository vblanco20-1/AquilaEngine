#pragma once
//#include <PrecompiledHeader.h>
#include <stdint.h>
#include "entt/entt.hpp"



#include <DirectXMath.h>

using namespace DirectX;
using EntityID = std::uint64_t;
using ECS_Registry = entt::Registry<std::uint64_t>;


class ECS_GameWorld;
extern ECS_GameWorld * GameWorld;

struct PositionComponent {
	XMFLOAT3 Position;
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
	float rate;
};
struct ScaleComponent {
	XMVECTOR Scale3D;
	
};
struct RotationComponent {
	XMVECTOR RotationAxis;
	float Angle;
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

