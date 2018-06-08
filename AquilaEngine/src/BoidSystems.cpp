#include "BoidSystems.h"
#include "libmorton/morton.h"
#include <execution>
#include <algorithm>
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
	loc.x = (short int)XMVectorGetX(p);//position.Position.x / GRID_DIMENSIONS;
	loc.y = (short int)XMVectorGetY(p);//position.Position.y / GRID_DIMENSIONS;
	loc.z = (short int)XMVectorGetZ(p);//position.Position.z / GRID_DIMENSIONS;
	return loc;
}
uint64_t MortonFromGrid(GridVec Loc)
{
	const uint32_t x = Loc.x + INT16_MIN;
	const uint32_t y = Loc.y + INT16_MIN;
	const uint32_t z = Loc.z + INT16_MIN;

	return morton3D_64_encode(x, y, z);
}
void BoidMap::AddToGridmap(const PositionComponent & position, const BoidComponent & boid)
{
	GridVec loc = GridVecFromPosition(position);

	GridItem2 gr(MortonFromGrid(loc));
	gr.boid = boid;
	gr.grid = loc;
	//gr.morton = MortonFromGrid(loc);
	gr.pos = XMLoadFloat3(&position.Position);
	Mortons.push_back(gr);

	return;
	//auto search = Grid.find(loc);
	//if (search != Grid.end())
	//{
	//	search->second.boids.push_back({ boid,position });
	//}
	//else
	//{
	//	GridBucket bucket;
	//	bucket.boids.reserve(10);
	//	bucket.boids.push_back({ boid,position });
	//	Grid[loc] = std::move(bucket);
	//}
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

void BoidMap::Foreach_EntitiesInGrid_Morton(const GridVec & loc, std::function<void(GridItem2&)> Body)
{
	//GridVec loc = GridVecFromPosition(Position);

	uint64_t morton = MortonFromGrid(loc);
	GridItem2 test;
	test.morton = morton;
	auto samemorton = [](const GridItem2&lhs, const GridItem2& rhs) { return lhs.morton < rhs.morton; };
	auto lower = std::lower_bound(Mortons.begin(), Mortons.end(), test,samemorton);
	auto upper = std::upper_bound(Mortons.begin(), Mortons.end(), test,samemorton);

	if (lower != Mortons.end() && upper != Mortons.end())
	{
		std::for_each(lower, upper, Body);
	}
	//auto search = Grid.find(loc);
	//if (search != Grid.end())
	//{
	//	for (auto g : search->second.boids)
	//	{
	//		Body(g);
	//	}
	//}
}

void BoidMap::Foreach_EntitiesInRadius(float radius, const PositionComponent & Position, std::function<void(GridItem&)> Body)
{
	const float radSquared = radius * radius;
	

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

void BoidMap::Foreach_EntitiesInRadius_Morton(float radius, const XMVECTOR & position, std::function<void(GridItem2&)> Body)
{
	const float radSquared = radius * radius;	

	const XMVECTOR Pos = position;

	const GridVec MinGrid = GridVecFromVector(Pos - XMVECTOR{ radius,radius,radius,0.0f });

	const GridVec MaxGrid = GridVecFromVector(Pos + XMVECTOR{ radius,radius,radius,0.0f });

	for (int x = MinGrid.x; x <= MaxGrid.x; x++) {
		for (int y = MinGrid.y; y <= MaxGrid.y; y++) {
			for (int z = MinGrid.z; z <= MaxGrid.z ; z++) {
				const GridVec SearchLoc{ x, y, z };	
				
				const GridItem2 test(MortonFromGrid(SearchLoc));
				
				auto compare_morton = [](const GridItem2&lhs, const GridItem2& rhs) { return lhs.morton < rhs.morton; };

				auto lower = std::lower_bound(Mortons.begin(), Mortons.end(), test, compare_morton);

				if (lower != Mortons.end())
				{
					auto upper = std::upper_bound(lower, Mortons.end(), test, compare_morton);

					if (upper != Mortons.end())
					{
						std::for_each(lower, upper, [&](GridItem2&item) {

							const XMVECTOR Dist = XMVector3LengthSq(Pos - item.pos);
							if (XMVectorGetX(Dist) < radSquared)
							{
								Body(item);
							}
						});
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

bool operator<(const GridItem2&a, const GridItem2&b)
{
	if (a.morton == b.morton)
	{
		return XMVectorGetX(XMVector3LengthSq(a.pos)) < XMVectorGetX(XMVector3LengthSq(b.pos));
	}
	else
	{
		return a.morton < b.morton;
	}
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
	auto Boidview = registry.view<PositionComponent, BoidComponent>(entt::persistent_t{});

	boidref.map->Mortons.clear();
	boidref.map->Mortons.reserve(Boidview.size());

		Boidview.each([&, dt](auto entity, const PositionComponent & campos, const BoidComponent & boid) {
		boidref.map->AddToGridmap(campos, boid);
		individualiterations++;
	});

		std::sort(std::execution::par, boidref.map->Mortons.begin(), boidref.map->Mortons.end(), [](const GridItem2&a, const GridItem2&b) {
			if (a.morton == b.morton)
			{
				return XMVectorGetX(XMVector3LengthSq(  a.pos)) < XMVectorGetX(XMVector3LengthSq(b.pos));
			}
			else
			{
				return a.morton < b.morton;
			}
		
		});
}
