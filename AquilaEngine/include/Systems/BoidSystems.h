#pragma once
//#include <PrecompiledHeader.h>
#include "ECSCore.h"
#define SPP_USE_SPP_ALLOC 1
#include <sparsepp/spp.h>
#include <atomic>
struct BoidComponent {
	char Type;
	char Faction;
	char Flags;
	EntityID SelfEntity;
};

struct GridVec {
	short int x;
	short int y;
	short int z;


};
struct GridItem2 {
	//BoidComponent boid;
	XMVECTOR pos;
	GridVec grid;
	uint64_t morton;
	
	GridItem2(uint64_t morton_code) : morton(morton_code) {}
	GridItem2() {};
};

bool operator==(const GridVec&a, const GridVec&b);
bool operator!=(const GridVec&a, const GridVec&b);
bool operator<(const GridItem2&a, const GridItem2&b);

struct GridHash {
	size_t  hashint(uint32_t a) const;

	std::size_t operator()(GridVec const& s) const noexcept;
};
const float GRID_DIMENSIONS = 5;
using GridItem = std::pair<BoidComponent, PositionComponent>;

struct GridHashmark {
	uint64_t morton;
	uint32_t start_idx;
	uint32_t stop_idx;
};
bool operator==(const GridHashmark&a, const GridHashmark&b);
struct GridBucket {
	std::vector<GridItem> boids;
};
struct Hash64 {
	size_t operator()(uint64_t k) const { return (k ^ 14695981039346656037ULL) * 1099511628211ULL; }
};
struct BoidMap {

	std::vector<GridItem2> Mortons;
	//std::vector<uint64_t> Mortons;
	//spp::sparse_hash_map<GridVec, GridBucket, GridHash> Grid;

	//spp::sparse_hash_map<uint64_t, GridHashmark, Hash64> MortonGrid;
	std::vector<GridHashmark> MortonArray;
	
	BoidMap() {};

	std::atomic_uint32_t MortonIdx;

	bool Binary_Find_Hashmark(GridHashmark &outHashmark, const size_t start, const size_t end, const uint64_t morton) const;
	GridVec GridVecFromPosition(const PositionComponent & position);
	GridVec GridVecFromVector(const XMVECTOR & position);
	void AddToGridmap(const PositionComponent & position, const BoidComponent & boid);
	void AddToGridmap(const XMVECTOR & pos, const BoidComponent & boid, size_t index);

	

	void Foreach_EntitiesInGrid(const PositionComponent & Position, std::function<void(GridItem&)> &&Body);

	void Foreach_EntitiesInGrid_Morton(const GridVec & loc, std::function<void(GridItem2&)>&&Body);


	void Foreach_EntitiesInRadius(float radius, const PositionComponent & Position, std::function<void(GridItem&)> &&Body);
	void Foreach_EntitiesInRadius_Morton(float radius, const XMVECTOR & position, std::function<bool(const GridItem2&)> &&Body);

};

struct BoidReferenceTag {
	BoidMap * map;
};
static int iterations{ 0 };
static int individualiterations{ 0 };
struct BoidHashSystem : public System {

	BoidHashSystem() { uses_threading = true; };

	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent);;

	virtual void update(ECS_Registry &registry, float dt);
	virtual void update(ECS_GameWorld & world) override;
	void initial_fill(ECS_GameWorld& world);
	void sort_structures(ECS_GameWorld& world);
};
