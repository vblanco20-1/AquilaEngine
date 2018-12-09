
#include "SimpleProfiler.h"


#include "BoidSystems.h"
#include "libmorton/morton.h"
#include <execution>
#include <algorithm>
#include <Remotery.h>
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

	
	gr.boid = boid;
	gr.grid = loc;
	
	gr.pos = XMLoadFloat3(&position.Position);
	Mortons[MortonIdx++] = gr;	

	return;	
}
void BoidMap::AddToGridmap(const XMVECTOR & pos, const BoidComponent & boid,size_t index)
{
	GridVec loc = GridVecFromVector(pos);

	GridItem2 gr(MortonFromGrid(loc));
	gr.boid = boid;
	gr.grid = loc;
	//gr.morton = MortonFromGrid(loc);
	gr.pos = pos;
	//Mortons[MortonIdx++] = gr;
	Mortons[index] = gr;
	//Mortons.push_back(gr);

	return;
}
void BoidMap::Foreach_EntitiesInGrid(const PositionComponent & Position, std::function<void(GridItem&)> Body)
{
	//GridVec loc = GridVecFromPosition(Position);
	//auto search = Grid.find(loc);
	//if (search != Grid.end())
	//{
	//	for (auto g : search->second.boids)
	//	{
	//		Body(g);
	//	}
	//}
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

void BoidMap::Foreach_EntitiesInRadius_Morton(float radius, const XMVECTOR & position, std::function<void(const GridItem2&)> Body)
{
	const float radSquared = radius * radius;	

	const XMVECTOR Pos = position;

	//calculate the minimum grid we are going to visit
	const GridVec MinGrid = GridVecFromVector(Pos - XMVECTOR{ radius,radius,radius,0.0f });
	//calculate the maximum grid we are going to visit
	const GridVec MaxGrid = GridVecFromVector(Pos + XMVECTOR{ radius,radius,radius,0.0f });

	//iterate that segment beetween min and max (3d)
	for (int x = MinGrid.x; x <= MaxGrid.x; x++) {
		for (int y = MinGrid.y; y <= MaxGrid.y; y++) {
			for (int z = MinGrid.z; z <= MaxGrid.z ; z++) {

				const GridVec SearchLoc{ x, y, z };					
				
				
				//prepare a hashmark with the morton code for the lower bound search
				GridHashmark testhash;
				testhash.morton = MortonFromGrid(SearchLoc);
				//search the morton tile array for the morton that fits
				auto compare_morton_array = [](const GridHashmark&lhs, const GridHashmark& rhs) { return lhs.morton < rhs.morton; };

				const auto lower = std::lower_bound(MortonArray.begin(), MortonArray.end(), testhash, compare_morton_array);
				
				if (lower != MortonArray.end())
				{
					//iterate the morton ordered segment of the cubes
					const size_t mstart = lower->start_idx;
					const size_t mend = lower->stop_idx;
					
					for (int i = mstart; i < mend; i++)
					{
						const GridItem2 & item = Mortons[i];
						const XMVECTOR Dist = XMVector3LengthSq(Pos - item.pos);
						if (XMVectorGetX(Dist) < radSquared)
						{
							Body(item);
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

ecs::Task BoidHashSystem::schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent)
{
	const float dt = get_delta_time(registry);
	ecs::Task task = task_engine.silent_emplace([&, dt]() {

		update(registry, dt);
	});


	task.name("Boid Hash system");
	//run after the parent
	task.gather(parent);
	return std::move(task);
}

void BoidHashSystem::update(ECS_Registry &registry, float dt)
{
	rmt_ScopedCPUSample(BoidHashSystem, 0);

		SCOPE_PROFILE("Boid Hash System All");

		iterations++;
		if (!registry.has<BoidReferenceTag>())
		{
			auto player = registry.create();
			BoidMap * map = new BoidMap();
			auto tag = registry.get_tag_entity();
			registry.assign<BoidReferenceTag>(tag,map);
		}

		//get the "boid data" pointer
		BoidReferenceTag & boidref = registry.get<BoidReferenceTag>();
		//grab a view for Transform and Boid entities
		auto Boidview = registry.persistent_view<TransformComponent, BoidComponent>();

		
		//reset data structures
		//boidref.map->Mortons.clear();
		//boidref.map->Mortons.reserve(Boidview.size());
		std::vector<size_t> indices;
		{
			rmt_ScopedCPUSample(DataStructureInitialization, 0);
			boidref.map->Mortons.resize(Boidview.size());

			boidref.map->MortonArray.clear();

			boidref.map->MortonArray.reserve(Boidview.size() / 20);
			
			indices.resize(Boidview.size());
			boidref.map->MortonIdx = 0;

			boidref.map->Mortons.resize(Boidview.size());
			//copy every entity of the view into the Mortons array (calculates morton too)


			int i = 0;

			for (size_t & dx : indices)
			{
				dx = i;
				i++;
			}
		}
	{
			rmt_ScopedCPUSample(InitialFill, 0);
		SCOPE_PROFILE("Boid Initial Fill");
		
		
		//std::for_each(std::execution::par,Boidview.begin(), Boidview.end(), [&](const auto entity) {
		std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), [&](const size_t idx) {
		//for (size_t & idx : indices)
		//{
			//this is done in as many threads as the STL decides, usually all
			auto[t, boid] = Boidview.get<TransformComponent, BoidComponent>(Boidview.data()[idx]);// entity);
		
			boidref.map->AddToGridmap(t.position, boid, idx);
			
		});
	}

	{
		
		if (boidref.map->Mortons.size() > 0)
		{
			{
				rmt_ScopedCPUSample(MortonSort, 0);
				SCOPE_PROFILE("Boid Hash Morton sort");


				//parallel sort all entities by morton code
				std::sort(std::execution::par,boidref.map->Mortons.begin(), boidref.map->Mortons.end(), [](const GridItem2&a, const GridItem2&b) {

					if (a.morton == b.morton)
					{
						//use the lengt of the vector to sort, as at this point it doesnt matter much
						return XMVectorGetX(XMVector3LengthSq(a.pos)) < XMVectorGetX(XMVector3LengthSq(b.pos));
					}
					else
					{
						//sort by morton
						return a.morton < b.morton;
					}
				});
			}
			{
				
				rmt_ScopedCPUSample(MortonHash, 0);
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
						boidref.map->MortonArray.push_back(LastMark);						
						LastGrid = NewGrid;
						LastMark.start_idx = i;
					}
				}
			}
		}
	}
		
}
