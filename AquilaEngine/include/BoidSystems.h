#pragma once
//#include <PrecompiledHeader.h>
#include "ECSCore.h"
#define SPP_USE_SPP_ALLOC 1
#include <sparsepp/spp.h>

struct BoidComponent {
	int Type;
	int Faction;
	int Flags;
	EntityID SelfEntity;
};

struct GridVec {
	short int x;
	short int y;
	short int z;


};
bool operator==(const GridVec&a, const GridVec&b);

struct GridHash {
	size_t  hashint(uint32_t a) const;

	std::size_t operator()(GridVec const& s) const noexcept;
};
const float GRID_DIMENSIONS = 20;
using GridItem = std::pair<BoidComponent, PositionComponent>;


struct GridBucket {
	std::vector<GridItem> boids;
};
struct BoidMap {
	spp::sparse_hash_map<GridVec, GridBucket, GridHash> Grid;
	BoidMap() {};

	GridVec GridVecFromPosition(const PositionComponent & position);
	GridVec GridVecFromVector(const XMVECTOR & position);
	void AddToGridmap(const PositionComponent & position, const BoidComponent & boid);

	void Foreach_EntitiesInGrid(const PositionComponent & Position, std::function<void(GridItem&)> Body);
	void Foreach_EntitiesInRadius(float radius, const PositionComponent & Position, std::function<void(GridItem&)> Body);

};

struct BoidReferenceTag {
	BoidMap * map;
};
static int iterations{ 0 };
static int individualiterations{ 0 };
struct BoidHashSystem : public System {


	virtual void update(ECS_Registry &registry, float dt);
};
