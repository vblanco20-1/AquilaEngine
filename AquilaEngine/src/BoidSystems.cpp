
#include "SimpleProfiler.h"


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

bool BoidMap::Binary_Find_Hashmark(GridHashmark &outHashmark, const size_t start, const size_t end, const uint64_t morton) const
{
	if ((end - start) < 10)
	{
		for (auto i = start; i < end; i++)
		{
			if (MortonArray[i].morton == morton)
			{
				outHashmark = MortonArray[i];
				return true;
			}
		}		
	}
	else
	{
		size_t mid = start + ( (end - start) / 2);
		GridHashmark midpoint = MortonArray[mid];
		if (midpoint.morton > morton)
		{
			return Binary_Find_Hashmark(outHashmark,start, mid, morton);
		}
		else if(midpoint.morton < morton)
		{
			return Binary_Find_Hashmark(outHashmark,mid, end, morton);
		}
		else
		{
			outHashmark = midpoint;
			return true;
		}
	}

	
	
	return false;
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
				//
				//auto search = std::lower_bound(Mortons.begin(), Mortons.end(), test.morton, compare_morton);//MortonGrid.find(test.morton);
				//if (search != MortonArray.end())
				//{
				//	for (int i = search->second.start_idx; i < search->second.stop_idx; i++)
				//	{
				//		GridItem2 & item = Mortons[i];
				//		const XMVECTOR Dist = XMVector3LengthSq(Pos - item.pos);
				//		if (XMVectorGetX(Dist) < radSquared)
				//		{
				//			Body(item);
				//		}
				//
				//		//Body(g);
				//	}
				//}
				GridHashmark testhash;
				testhash.morton = MortonFromGrid(SearchLoc);
				auto compare_morton_array = [](const GridHashmark&lhs, const GridHashmark& rhs) { return lhs.morton < rhs.morton; };
				auto lower = std::lower_bound(MortonArray.begin(), MortonArray.end(), testhash, compare_morton_array);

				GridHashmark found;

				//if(Binary_Find_Hashmark(found,0,MortonArray.size(), MortonFromGrid(SearchLoc)))				//if (lower != MortonArray.end())
				//{
				//	if (lower->start_idx == found.start_idx)
				//	{
				if (lower != MortonArray.end())
				{

						size_t mstart = /*found.start_idx;*/lower->start_idx;
						size_t mend = /*found.stop_idx;//*/lower->stop_idx;
						//auto upper = std::upper_bound(lower, Mortons.end(), test, compare_morton);
						//
						//if (upper != Mortons.end())
						//{
						//	std::for_each(lower, upper, [&](GridItem2&item) {
						for (int i = mstart; i < mend; i++)
						{
							GridItem2 & item = Mortons[i];
							const XMVECTOR Dist = XMVector3LengthSq(Pos - item.pos);
							if (XMVectorGetX(Dist) < radSquared)
							{
								Body(item);
							}
						}
					//}
							
						//});
					//}
				}

				//auto lower = std::lower_bound(Mortons.begin(), Mortons.end(), test, compare_morton);
				//
				//if (lower != Mortons.end())
				//{
				//	auto upper = std::upper_bound(lower, Mortons.end(), test, compare_morton);
				//
				//	if (upper != Mortons.end())
				//	{
				//		std::for_each(lower, upper, [&](GridItem2&item) {
				//
				//			const XMVECTOR Dist = XMVector3LengthSq(Pos - item.pos);
				//			if (XMVectorGetX(Dist) < radSquared)
				//			{
				//				Body(item);
				//			}
				//		});
				//	}
				//}
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

bool operator!=(const GridVec&a, const GridVec&b)
{
	return !(a == b);
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

bool operator==(const GridHashmark&a, const GridHashmark&b)
{
	return a.morton == b.morton;
}

void BoidHashSystem::update(ECS_Registry &registry, float dt)
{
	
		SCOPE_PROFILE("Boid Hash System All");

		iterations++;
		if (!registry.has<BoidReferenceTag>())
		{
			auto player = registry.create();
			BoidMap * map = new BoidMap();
			registry.assign<BoidReferenceTag>(entt::tag_t{}, player, map);
		}

		BoidReferenceTag & boidref = registry.get<BoidReferenceTag>();
		//boidref.map = &myMap;
		//boidref.map->Grid.clear();
		//boidref.map->MortonGrid.clear();
		auto Boidview = registry.view<PositionComponent, BoidComponent>(entt::persistent_t{});

		boidref.map->Mortons.clear();
		boidref.map->Mortons.reserve(Boidview.size());
		boidref.map->MortonArray.clear();
		boidref.map->MortonArray.reserve(Boidview.size()/20);

	{
		SCOPE_PROFILE("Boid Initial Fill");

		Boidview.each([&, dt](auto entity, const PositionComponent & campos, const BoidComponent & boid) {
			boidref.map->AddToGridmap(campos, boid);
			individualiterations++;
		});
	}
	{
		
		if (boidref.map->Mortons.size() > 0)
		{
			{
				SCOPE_PROFILE("Boid Hash Morton sort");
				std::sort(std::execution::par, boidref.map->Mortons.begin(), boidref.map->Mortons.end(), [](const GridItem2&a, const GridItem2&b) {
					if (a.morton == b.morton)
					{
						return XMVectorGetX(XMVector3LengthSq(a.pos)) < XMVectorGetX(XMVector3LengthSq(b.pos));
					}
					else
					{
						return a.morton < b.morton;
					}
				});
			}
			{
				SCOPE_PROFILE("Boid Hash Morton hash");
			
				//auto& mgrid = boidref.map->MortonGrid;
				//mgrid.reserve(Boidview.size()/100);
				GridVec LastGrid = boidref.map->Mortons[0].grid;
				GridHashmark LastMark{ 0,0 };
			
				for (size_t i = 1; i < boidref.map->Mortons.size(); i++)
				{
					GridVec NewGrid = boidref.map->Mortons[i].grid;
					if (LastGrid != NewGrid)
					{
						LastMark.stop_idx = i;
						LastMark.morton = MortonFromGrid(LastGrid);
						boidref.map->MortonArray.push_back(LastMark);
						//mgrid[MortonFromGrid(LastGrid)] = LastMark;
						LastGrid = NewGrid;
						LastMark.start_idx = i;
					}
				}
			}

		}
	}
		
}
