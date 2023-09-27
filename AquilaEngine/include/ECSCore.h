#pragma once
#include <PrecompiledHeader.h>

#include "decs.h"
#include "taskflow/core/flow_builder.hpp"
#include "taskflow/algorithm/for_each.hpp"
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
	float delta_time{0.f};
	uint64_t frameNumber{0};
};

struct ExplosionFXComponent {
	
	float elapsed = 0;
};

struct Transform {
	XMVECTOR position;
	XMVECTOR rotationQuat;
	XMVECTOR scale;

	Transform() : rotationQuat(XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0)), scale(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f)) {
	}
};

struct TransformComponent {
	Transform* tf;
	
	void init() {
		tf = new Transform;
	}

	~TransformComponent() {
		delete tf;
	}
};

struct StaticTransform {
};




struct  System {
	virtual ~System() {}	

	virtual void update(ECS_GameWorld & world);
	virtual void update_par(ECS_GameWorld& world, tf::Subflow& sf){update(world); };

	virtual PureSystemBase* getAsPureSystem(){return nullptr;};
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
	RenderMatrixComponent(XMMATRIX && other)
	{
		Matrix = other;
	}
	RenderMatrixComponent() = default;
	XMMATRIX Matrix;
};

//struct LocalMatrix {
//	LocalMatrix(XMMATRIX&& other)
//	{
//		Matrix = other;
//	}
//	LocalMatrix() = default;
//	XMMATRIX Matrix;
//};

struct CubeRendererComponent{
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

struct SceneNode {
	XMMATRIX matrix;
	SceneNode* parent{};
	SceneNode* firstChild{};
	SceneNode* nextBrother{};
	SceneNode* prevBrother;

	Transform* transform{};
};

struct SceneNodeComponent {
	
	SceneNode* node;
	Transform* transform;

	void init(Transform* tf) {
		node = new SceneNode;
		node->transform = tf;
		transform = tf;
	}
	~SceneNodeComponent() {
		//disconnect properly
		if (node)
		{
			//separate from parent
			detachParent();


			SceneNode* c = node->firstChild;
			//unparent children
			while (c) {
				SceneNode* next = c->nextBrother;
				c->nextBrother = nullptr;
				c->prevBrother = nullptr;
				c->parent = nullptr;
				c = next;
			}
			


			//
			//for (auto& p : node->children) {
			//	p->parent = nullptr;
			//}
			//kill node
			delete node;
		}
	}

	void detachParent() {
		if (node->parent) {
			//its a circular linked list, if brother == node, its a single-child
			if (node->nextBrother == node) {
				node->parent->firstChild = nullptr;
			}
			else {
				node->prevBrother->nextBrother = node->nextBrother;
				node->nextBrother->prevBrother = node->prevBrother;
			}

			node->nextBrother = nullptr;
			node->prevBrother = nullptr;
			node->parent = nullptr;
		}
	}

	void setParent(SceneNode* parent) {
		if(node->parent == parent) return; //already a child of this


		detachParent();

		//parent has no children
		if (parent->firstChild == nullptr) {
			parent->firstChild = node;
			node->prevBrother = node;
			node->nextBrother = node;
		}
		else {
			//insert after the firstchild
			node->nextBrother = parent->firstChild->nextBrother;
			node->prevBrother = parent->firstChild;

			node->prevBrother->nextBrother = node;
			node->nextBrother->prevBrother = node;
		}

		node->parent = parent;

		//int holeIdx = -1;
		//for (int i = 0; i < parent->children.size(); i++) {
		//	auto p = parent->children[i];
		//	if (p == node) return; //already in child list? 
		//	else if (p == nullptr) //parent has a hole, fill it
		//	{
		//		holeIdx = i; break;
		//	}
		//}
		//
		//if (holeIdx >= 0) {
		//	parent->children[holeIdx] = node;
		//}
		//else {
		//	parent->children.push_back(node);
		//}
	}
}; 


constexpr auto f = sizeof(SceneNodeComponent);
struct TransformParentComponent {
	decs::CachedRef<RenderMatrixComponent> ParentTransform;
	decs::EntityID Parent;
};

template<typename F>
void parallel_for_chunk(std::vector<DataChunk*>& chunks, F&& functor) {
	std::for_each(std::execution::par, chunks.begin(), chunks.end(), functor);
}

template<typename F>
void parallel_for_chunk(tf::Subflow& sf,std::vector<DataChunk*>& chunks, F&& functor) {
	sf.for_each(chunks.begin(), chunks.end(), functor);
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