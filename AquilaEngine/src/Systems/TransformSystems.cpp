#include <PrecompiledHeader.h>

#include "Systems/TransformSystems.h"
#include "GameWorld.h"
#include "Systems/RenderSystems.h"




void ecs::system::UpdateTransform::update_root(ECS_GameWorld& world)
{
	ZoneScopedN("Update Root Transforms");
	Query FullTransform;
	FullTransform.with<RenderMatrixComponent, TransformComponent>();
	FullTransform.build(); 
	
	static std::vector<DataChunk*> full_chunk_cache;
	full_chunk_cache.clear();

	world.registry_decs.gather_chunks(FullTransform, full_chunk_cache);
	
	parallel_for_chunk(full_chunk_cache, [&](DataChunk* chnk) {

		ZoneScopedN("Transform chunk");
		
		auto tfarray = get_chunk_array<TransformComponent>(chnk);		
		auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);

		if (tfarray.version() > matarray.version()) {

			update_root_transform(chnk);

			world.mark_components_changed(matarray);
		}
	});
}

void ecs::system::UpdateTransform::update_hierarchy(ECS_GameWorld& world, tf::Subflow& sf)
{
	ZoneScopedN("Update Hierarchy Transforms");

	Query ChildTransform;
	ChildTransform.with<RenderMatrixComponent, TransformComponent, TransformParentComponent>();
	ChildTransform.build();



	static std::vector<DataChunk*> child_chunk_cache;
	child_chunk_cache.clear();

	world.registry_decs.gather_chunks(ChildTransform, child_chunk_cache);	

	float zero = 0;
	XMVECTOR zerovec = XMLoadFloat(&zero);	
	
	parallel_for_chunk(sf,child_chunk_cache, [&](DataChunk* chnk) {

		update_children_transform(chnk, world);
	});
	sf.join();
}

decs::PureSystemBase* ecs::system::UpdateTransform::update_root_puresys()
{
	static auto puresys = []() {

		Query query;
		query.with<RenderMatrixComponent, TransformComponent>();
		query.build();

		return decs::make_pure_system_chunk(query, [](void* context, DataChunk* chnk) {

			ECS_GameWorld& world = *reinterpret_cast<ECS_GameWorld*>(context);
			
			ZoneScopedN("Transform chunk root - pure");

			//auto tfarray = get_chunk_array<TransformComponent>(chnk);
			//auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);

			//if (tfarray.version() > matarray.version())
			{

				update_root_transform(chnk);

				//world.mark_components_changed(matarray);
			}
		});
	}();

	return &puresys;
}

decs::PureSystemBase* ecs::system::UpdateTransform::update_hierarchy_puresys()
{
	static auto puresys = []() {

		Query query;
		query.with<RenderMatrixComponent, TransformComponent, TransformParentComponent>();
		query.build();

		return decs::make_pure_system_chunk(query, [](void* context, DataChunk* chnk) {

			ECS_GameWorld& world = *reinterpret_cast<ECS_GameWorld*>(context);

			ZoneScopedN("Transform children - pure");

			update_children_transform(chnk, world);
		});
	}();

	return &puresys;
}

void update_root_transform_arrays(DataChunk* chnk,
	RenderMatrixComponent* __restrict matarray,
	TransformComponent* __restrict transfarray,
	PositionComponent* __restrict posarray)
{
	for (int i = chnk->count() - 1; i >= 0; i--)
	{
		TransformComponent& t = transfarray[i];
		if (posarray)
		{
			PositionComponent& p = posarray[i];
			ecs::system::UpdateTransform::apply_position(t, p);
		}

		ecs::system::UpdateTransform::build_matrix(t, matarray[i]);
	}
}
void update_root_transform(DataChunk* chnk)
{
	ZoneScopedNC("Transform chunk execute: ", tracy::Color::Red);

	auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);
	auto transfarray = get_chunk_array<TransformComponent>(chnk);
	auto posarray = get_chunk_array<PositionComponent>(chnk);

	const bool bHasPosition = posarray.chunkOwner == chnk;

	update_root_transform_arrays(chnk, matarray.data, transfarray.data,
		bHasPosition ? posarray.data : nullptr);
}
void update_children_transform_arrays(decs::ECSWorld* world, DataChunk* chnk ,
	RenderMatrixComponent* __restrict matarray,
	TransformComponent* __restrict transfarray,
	TransformParentComponent* __restrict parentarray)
{
	//unrolled for perf
	int i = 0;
	for (i; i < (chnk->count() & ~3); i += 4) {
	
	
		TransformParentComponent& tpc1 = parentarray[i + 0];
		TransformParentComponent& tpc2 = parentarray[i + 1];
		TransformParentComponent& tpc3 = parentarray[i + 2];
		TransformParentComponent& tpc4 = parentarray[i + 3];
	
	
		const XMMATRIX* m1 = &(tpc1.ParentTransform.get_from(world, tpc1.Parent))->Matrix;
		_mm_prefetch((const char*)(m1), 0);
	
		const XMMATRIX* m2 = &(tpc2.ParentTransform.get_from(world, tpc2.Parent))->Matrix;
		_mm_prefetch((const char*)(m2), 0);
	
		const XMMATRIX* m3 = &(tpc3.ParentTransform.get_from(world, tpc3.Parent))->Matrix;
		_mm_prefetch((const char*)(m3), 0);
	
		const XMMATRIX* m4 = &(tpc4.ParentTransform.get_from(world, tpc4.Parent))->Matrix;		
		_mm_prefetch((const char*)(m4), 0);
	
		matarray[i + 0].Matrix *= *m1;
		matarray[i + 1].Matrix *= *m2;
		matarray[i + 2].Matrix *= *m3;
		matarray[i + 3].Matrix *= *m4;	
	}

	for (i; i < (chnk->count()); i++) {

		TransformParentComponent& tpc = parentarray[i];

		const XMMATRIX& parent_matrix = (tpc.ParentTransform.get_from(world, tpc.Parent))->Matrix;

		matarray[i].Matrix *= parent_matrix;
	}
}

void update_children_transform_arrays(decs::ECSWorld* world, DataChunk* chnk,
	RenderMatrixComponent* __restrict matarray,
	LocalMatrix* __restrict transfarray,
	TransformParentComponent* __restrict parentarray)
{
	//unrolled for perf
	int i = 0;
	for (i; i < (chnk->count() & ~3); i += 4) {


		TransformParentComponent& tpc1 = parentarray[i + 0];
		TransformParentComponent& tpc2 = parentarray[i + 1];
		TransformParentComponent& tpc3 = parentarray[i + 2];
		TransformParentComponent& tpc4 = parentarray[i + 3];


		const XMMATRIX* m1 = &(tpc1.ParentTransform.get_from(world, tpc1.Parent))->Matrix;
		_mm_prefetch((const char*)(m1), 0);

		const XMMATRIX* m2 = &(tpc2.ParentTransform.get_from(world, tpc2.Parent))->Matrix;
		_mm_prefetch((const char*)(m2), 0);

		const XMMATRIX* m3 = &(tpc3.ParentTransform.get_from(world, tpc3.Parent))->Matrix;
		_mm_prefetch((const char*)(m3), 0);

		const XMMATRIX* m4 = &(tpc4.ParentTransform.get_from(world, tpc4.Parent))->Matrix;
		_mm_prefetch((const char*)(m4), 0);

		matarray[i + 0].Matrix = transfarray[i + 0].Matrix * *m1;
		matarray[i + 1].Matrix = transfarray[i + 1].Matrix * *m2;
		matarray[i + 2].Matrix = transfarray[i + 2].Matrix * *m3;
		matarray[i + 3].Matrix = transfarray[i + 3].Matrix * *m4;
	}

	for (i; i < (chnk->count()); i++) {

		TransformParentComponent& tpc = parentarray[i];

		const XMMATRIX& parent_matrix = (tpc.ParentTransform.get_from(world, tpc.Parent))->Matrix;

		matarray[i].Matrix = transfarray[i].Matrix * parent_matrix;
	}
}

void update_children_transform(DataChunk* chnk, ECS_GameWorld& world)
{
	ZoneScopedNC("Transform chunk execute: children", tracy::Color::Orange);

	auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);
	auto transfarray = get_chunk_array<TransformComponent>(chnk);
	auto localmats = get_chunk_array<LocalMatrix>(chnk);

	auto parentarray = get_chunk_array<TransformParentComponent>(chnk);

	if (localmats.valid()) {
		//unrolled for perf
		update_children_transform_arrays(&world.registry_decs, chnk, matarray.data, localmats.data, parentarray.data);
	}
	else {
		//unrolled for perf
		update_children_transform_arrays(&world.registry_decs, chnk, matarray.data, transfarray.data, parentarray.data);
	}
	
	world.mark_components_changed(matarray);	
}
