#pragma once
#include <PrecompiledHeader.h>

using EntityID = std::uint64_t;
using ECS_Registry = entt::Registry<std::uint64_t>;

using spp::sparse_hash_map;


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
struct PositionComponent {
	XMFLOAT3 Position;
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
	//XMMATRIX Matrix;
};
struct SpaceshipMovementComponent {
	XMVECTOR Velocity;
	XMVECTOR Heading;
	XMVECTOR Target;
	float speed;
};
struct PlayerInputTag {
	InputMap Input;
	//XMMATRIX Matrix;
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

struct BoidComponent {
	int Type;
	int Faction;
	int Flags;
	EntityID SelfEntity;
};