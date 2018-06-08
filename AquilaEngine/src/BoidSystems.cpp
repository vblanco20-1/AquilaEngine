#include "BoidSystems.h"

GridVec BoidMap::GridVecFromPosition(const PositionComponent & position)
{
	GridVec loc;
	loc.x = (int)(position.Position.x / GRID_DIMENSIONS);
	loc.y = (int)(position.Position.y / GRID_DIMENSIONS);
	loc.z = (int)(position.Position.z / GRID_DIMENSIONS);
	return loc;
}

GridVec BoidMap::GridVecFromVector(const XMVECTOR & position)
{
	GridVec loc;
	XMVECTOR p = position / GRID_DIMENSIONS;
	loc.x = (int)XMVectorGetX(p);//position.Position.x / GRID_DIMENSIONS;
	loc.y = (int)XMVectorGetX(p);//position.Position.y / GRID_DIMENSIONS;
	loc.z = (int)XMVectorGetX(p);//position.Position.z / GRID_DIMENSIONS;
	return loc;
}

void BoidMap::AddToGridmap(const PositionComponent & position, const BoidComponent & boid)
{
	GridVec loc = GridVecFromPosition(position);


	auto search = Grid.find(loc);
	if (search != Grid.end())
	{
		search->second.boids.push_back({ boid,position });
	}
	else
	{
		GridBucket bucket;
		bucket.boids.reserve(10);
		bucket.boids.push_back({ boid,position });
		Grid[loc] = std::move(bucket);
	}
}

void BoidMap::Foreach_EntitiesInGrid(const PositionComponent & Position, std::function<void(GridItem&)> Body)
{
	GridVec loc = GridVecFromPosition(Position);
	auto search = Grid.find(loc);
	if (search != Grid.end())
	{
		for (auto g : search->second.boids)
		{
			Body(g);
		}
	}
}

void BoidMap::Foreach_EntitiesInRadius(float radius, const PositionComponent & Position, std::function<void(GridItem&)> Body)
{
	const float radSquared = radius * radius;
	//GridVec Grid = GridVecFromPosition(Position);

	XMVECTOR Pos = XMLoadFloat3(&Position.Position);


	GridVec MinGrid = GridVecFromVector(Pos - XMVECTOR{ radius });

	GridVec MaxGrid = GridVecFromVector(Pos + XMVECTOR{ radius });

	for (int x = MinGrid.x; x <= MaxGrid.x; x++) {
		for (int y = MinGrid.y; y <= MaxGrid.y; y++) {
			for (int z = MinGrid.z; z <= MaxGrid.z; z++) {
				const GridVec SearchLoc{ x, y, z };

				auto search = Grid.find(SearchLoc);
				if (search != Grid.end())
				{
					for (auto g : search->second.boids)
					{
						XMVECTOR Otherpos = XMLoadFloat3(&g.second.Position);
						XMVECTOR Dist = XMVector3LengthSq(Pos - Otherpos);
						if (XMVectorGetX(Dist) < radSquared)
						{
							Body(g);
						}

					}

				}
			}
		}
	}
}

size_t GridHash::hashint(uint32_t a) const
{
	a -= (a << 6);
	a ^= (a >> 17);
	a -= (a << 9);
	a ^= (a << 4);
	a -= (a << 3);
	a ^= (a << 10);
	a ^= (a >> 15);
	return  (a ^ 2166136261U) * 16777619UL;
}

std::size_t GridHash::operator()(GridVec const& s) const noexcept
{
	int32_t h = s.x + (s.y << 16) * s.z;
	uint32_t tmp{ 0 };
	std::memcpy(&tmp, &h, sizeof(uint32_t));
	//size_t hash = ;
	return hashint(tmp);
}

bool operator==(const GridVec&a, const GridVec&b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

void BoidHashSystem::update(ECS_Registry &registry, float dt)
{
	iterations++;
	if (!registry.has<BoidReferenceTag>())
	{
		auto player = registry.create();
		BoidMap * map = new BoidMap();
		registry.assign<BoidReferenceTag>(entt::tag_t{}, player, map);
	}

	BoidReferenceTag & boidref = registry.get<BoidReferenceTag>();
	//boidref.map = &myMap;
	boidref.map->Grid.clear();
	registry.view<PositionComponent, BoidComponent>(entt::persistent_t{}).each([&, dt](auto entity, const PositionComponent & campos, const BoidComponent & boid) {
		boidref.map->AddToGridmap(campos, boid);
		individualiterations++;
	});
}
