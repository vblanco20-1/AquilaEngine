#include <PrecompiledHeader.h>

#include "SimpleProfiler.h"

 
#include "Systems/BoidSystems.h"
#include "libmorton/morton.h"
#include <execution>
#include <algorithm>

#include "GameWorld.h"
#include "ApplicationInfoUI.h"
//#include "ApplicationInfoUI.h"



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

	
	//gr.boid = boid;
	gr.grid = loc;
	
	gr.pos = XMLoadFloat3(&position.Position);
	Mortons[MortonIdx++] = gr;	

	return;	
}
void BoidMap::AddToGridmap(const XMVECTOR & pos, const BoidComponent & boid,size_t index)
{
	GridVec loc = GridVecFromVector(pos);

	GridItem2 gr(MortonFromGrid(loc));
	//gr.boid = boid;
	gr.grid = loc;
	//gr.morton = MortonFromGrid(loc);
	gr.pos = pos;
	//Mortons[MortonIdx++] = gr;
	//if (index >= Mortons.size())
	//{
	
	gr.pos = XMVectorSetW(gr.pos,XMVectorGetX(XMVector3LengthSq(gr.pos)));
	
	if (index == -1) {
		Mortons.push_back(gr);
	}
	else {
		Mortons[index] = gr;
	}

		
	//}
	//Mortons[index] = gr;
	//Mortons.push_back(gr);

	return;
}



void BoidMap::Foreach_EntitiesInGrid(const PositionComponent & Position, std::function<void(GridItem&)> &&Body)
{
	
}

void BoidMap::Foreach_EntitiesInGrid_Morton(const GridVec & loc, std::function<void(GridItem2&)>&& Body)
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

}

void BoidMap::Foreach_EntitiesInRadius(float radius, const PositionComponent & Position, std::function<void(GridItem&)> &&Body)
{
	//const float radSquared = radius * radius;
	//
	//
	//XMVECTOR Pos = XMLoadFloat3(&Position.Position);
	//
	//GridVec MinGrid = GridVecFromVector(Pos - XMVECTOR{ radius });
	//
	//GridVec MaxGrid = GridVecFromVector(Pos + XMVECTOR{ radius });
	//
	//for (int x = MinGrid.x; x <= MaxGrid.x; x++) {
	//	for (int y = MinGrid.y; y <= MaxGrid.y; y++) {
	//		for (int z = MinGrid.z; z <= MaxGrid.z; z++) {
	//			const GridVec SearchLoc{ x, y, z };
	//
	//			auto search = Grid.find(SearchLoc);
	//			if (search != Grid.end())
	//			{
	//				for (auto g : search->second.boids)
	//				{
	//					XMVECTOR Otherpos = XMLoadFloat3(&g.second.Position);
	//					XMVECTOR Dist = XMVector3LengthSq(Pos - Otherpos);
	//					if (XMVectorGetX(Dist) < radSquared)
	//					{
	//						Body(g);
	//					}
	//
	//				}
	//
	//			}
	//		}
	//	}
	//}
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

void BoidMap::Foreach_EntitiesInRadius_Morton(float radius, const XMVECTOR & position, std::function<bool(const GridItem2&)> &&Body)
{
	//if (MortonArray.size() == 0) return;

	const float radSquared = radius * radius;	

	const XMVECTOR Pos = position;

	//calculate the minimum grid we are going to visit
	const GridVec MinGrid = GridVecFromVector(Pos - XMVECTOR{ radius,radius,radius,0.0f });
	//calculate the maximum grid we are going to visit
	const GridVec MaxGrid = GridVecFromVector(Pos + XMVECTOR{ radius,radius,radius,0.0f });

	//search the morton tile array for the morton that fits
	auto compare_morton_array = [](const GridHashmark&lhs, const GridHashmark& rhs) { return lhs.morton < rhs.morton; };

	GridHashmark maxhash;
	maxhash.morton = MortonFromGrid(MaxGrid);

	GridHashmark minhash;
	minhash.morton = MortonFromGrid(MinGrid);

	//calculate bounds
	GridHashmark* lowbound = &MortonArray[0]; //std::lower_bound(&MortonArray[0], &MortonArray[MortonArray.size() - 1], minhash, compare_morton_array);
	GridHashmark* highbound = &MortonArray[MortonArray.size() - 1];// std::upper_bound(&MortonArray[0], &MortonArray[MortonArray.size() - 1], maxhash, compare_morton_array);
	
	//if (maxhash.morton != minhash.morton) {
	//	GridHashmark* lowbound = std::lower_bound(&MortonArray[0], &MortonArray[MortonArray.size() - 1], minhash, compare_morton_array);
	//	GridHashmark* highbound =  std::upper_bound(lowbound, &MortonArray[MortonArray.size() - 1], maxhash, compare_morton_array);
	//
	//}
	//iterate that segment beetween min and max (3d)
	for (int x = MinGrid.x; x <= MaxGrid.x; x++) {
		for (int y = MinGrid.y; y <= MaxGrid.y; y++) {
			for (int z = MinGrid.z; z <= MaxGrid.z ; z++) {

				const GridVec SearchLoc{ x, y, z };					
								
				//prepare a hashmark with the morton code for the lower bound search
				GridHashmark testhash;
				testhash.morton = MortonFromGrid(SearchLoc);

				const auto lower = MortonGrid.find(testhash.morton);//std::lower_bound(lowbound, highbound, testhash, compare_morton_array);

				if (lower != MortonGrid.end())//!= &MortonArray[MortonArray.size() - 1])
				{
					//iterate the morton ordered segment of the cubes
					//const size_t mstart = lower->start_idx;
					//const size_t mend = lower->stop_idx;

					const size_t mstart = lower->second.start_idx;
					const size_t mend = lower->second.stop_idx;
					
					for (int i = mstart; i < mend; i++)
					{
						const GridItem2 & item = Mortons[i];
						const XMVECTOR Dist = XMVector3LengthSq(Pos - item.pos);
						if (XMVectorGetX(Dist) < radSquared)
						{
							if (!Body(item)) {
								return;
							}
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


void BoidHashSystem::initial_fill(ECS_GameWorld& world)
{
	iterations++;

	//get the "boid data" pointer
	BoidReferenceTag& boidref = *world.registry_decs.get_singleton<BoidReferenceTag>(); 

	size_t count = 200000;

	{
		ZoneScopedNC("Accel Initialization", tracy::Color::Blue);
		boidref.map->Mortons.clear();

		boidref.map->Mortons.reserve(count);

		boidref.map->MortonArray.clear();
		for (auto m : boidref.map->MortonGrid) {
			m.second.start_idx = 0;
			m.second.stop_idx = 0;
		}
		//boidref.map->MortonGrid.clear();
		boidref.map->MortonArray.reserve(count / 20);


		boidref.map->MortonIdx = 0;

	}
	{
		ZoneScopedNC("Initial Fill", tracy::Color::Red);
		SCOPE_PROFILE("Boid Initial Fill");




		static std::vector<DataChunk*> chunk_cache;
		chunk_cache.clear();
		std::atomic<int> gridmap_indices = 0;
		int total_boids = 0;
		Query query;
		query.with<BoidComponent, TransformComponent>();
		query.build();


		{
			ZoneScopedNC("Boids Gather Archetypes", tracy::Color::Green);

			world.registry_decs.gather_chunks(query, chunk_cache);
			decs::adv::iterate_matching_archetypes(&world.registry_decs, query, [&](Archetype* arch) {

				for (auto chnk : arch->chunks) {
					total_boids += chnk->header.last;
					//chunk_cache.push_back(chnk);
				}
			});
		}
		world.registry_decs.get_singleton<ApplicationInfo>()->BoidEntities = total_boids;
		boidref.map->Mortons.resize(total_boids);

		std::for_each(std::execution::par, chunk_cache.begin(), chunk_cache.end(), [&](DataChunk* chnk) {

			ZoneScopedNC("Boids Execute Chunks", tracy::Color::Red);

			auto boids = get_chunk_array<BoidComponent>(chnk);
			auto transforms = get_chunk_array<TransformComponent>(chnk);

			//add atomic to reserve space
			int first_idx = gridmap_indices.fetch_add(chnk->header.last);

			for (int i = 0; i < chnk->header.last; i++)
			{
				boidref.map->AddToGridmap(transforms[i].position, boids[i], first_idx + i);
			}
		});

	}
}

void BoidHashSystem::sort_structures(ECS_GameWorld& world)
{
	//ECS_Registry& registry = world.registry_entt;

	//get the "boid data" pointer
	BoidReferenceTag& boidref = *world.registry_decs.get_singleton<BoidReferenceTag>(); //registry.get<BoidReferenceTag>();
	

	if (boidref.map->Mortons.size() > 0)
	{
		{
			ZoneScopedN("Morton Sort");
			//ZoneNamed(MortonSort, true);
			SCOPE_PROFILE("Boid Hash Morton sort");


			//parallel sort all entities by morton code
			std::sort(std::execution::par, boidref.map->Mortons.begin(), boidref.map->Mortons.end(), [](GridItem2& a, GridItem2& b) {

				if (a.morton == b.morton)
				{
					//use the lengt of the vector to sort, as at this point it doesnt matter much
					return XMVectorGetW(a.pos) < XMVectorGetW(b.pos);
					//return XMVectorGetX(XMVector3LengthSq(a.pos)) < XMVectorGetX(XMVector3LengthSq(b.pos));
				}
				else
				{
					//sort by morton
					return a.morton < b.morton;
				}
				});
		}
		{
			ZoneScopedN("Morton Hash");
			//ZoneNamedNC(MortonHash, "Hash", tracy::Color::Blue);
			SCOPE_PROFILE("Boid Hash Morton hash");

			//compact the entities array into the grid array, to speed up binary search
			GridVec LastGrid = boidref.map->Mortons[0].grid;
			GridHashmark LastMark{ 0,0 };
			for (size_t i = 1; i < boidref.map->Mortons.size(); i++)
			{
				GridVec NewGrid = boidref.map->Mortons[i].grid;
				if (LastGrid != NewGrid)
				{
					LastMark.stop_idx = i;
					LastMark.morton = MortonFromGrid(LastGrid);
					//boidref.map->MortonArray.push_back(LastMark);
					boidref.map->MortonGrid[LastMark.morton] = LastMark;
					//boidref.map->MortonGrid.insert({ LastMark.morton , LastMark });
					LastGrid = NewGrid;
					LastMark.start_idx = i;
				}
			}
		}
	}	
}

void BoidHashSystem::update(ECS_GameWorld & world)
{
	ZoneNamedNC(BoidHashSystem,"Boid Hash System", tracy::Color::Blue, true);
	
	SCOPE_PROFILE("Boid Hash System All");

	initial_fill(world);

	sort_structures(world);
}
