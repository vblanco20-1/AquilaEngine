#include <PrecompiledHeader.h>
#include "Systems/RenderSystems.h"
#include "GameWorld.h"
#include "ApplicationInfoUI.h"



// Clear the color and depth buffers.
void Clear(const FLOAT clearColor[4], FLOAT clearDepth, UINT8 clearStencil)
{
	Globals->g_d3dDeviceContext->ClearRenderTargetView(Globals->g_d3dRenderTargetView, clearColor);
	Globals->g_d3dDeviceContext->ClearDepthStencilView(Globals->g_d3dDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, clearStencil);
}




void ecs::system::CameraUpdate::update(ECS_GameWorld &world)
{
	//update(world.registry_entt, world.GetTime().delta_time);

	world.registry_decs.for_each([&](EntityID entity, PositionComponent& campos, CameraComponent& cam) {
		XMVECTOR eyePosition = XMVectorSet(campos.Position.x, campos.Position.y, campos.Position.z, 1);; //XMVectorSet(0, 0, -70, 1);
		XMVECTOR focusPoint = cam.focusPoint;// + XMVectorSet(CamOffset.x, CamOffset.y, 0.0f, 0.0f); //XMVectorSet(0, 0, 0, 1);
		XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);

		Globals->g_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
		Globals->g_d3dDeviceContext->UpdateSubresource(Globals->g_d3dConstantBuffers[CB_Frame], 0, nullptr, &Globals->g_ViewMatrix, 0, 0);

		//rotation.Angle += 90.0f * dt;
	});
}

bool IsVisibleFrustrumCull(const XMVECTOR& pos,const XMVECTOR &CamPos,const XMVECTOR& CamDir ) {
//	XMVECTOR pos = XMVector3Transform(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), matrix.Matrix);
	XMVECTOR ToCube = CamPos - pos;

	XMVECTOR V1 = ToCube;
	XMVECTOR V2 = CamDir;

	XMVECTOR L1 = XMVector3ReciprocalLength(V1);
	XMVECTOR L2 = XMVector3ReciprocalLength(V2);

	XMVECTOR Dot = XMVector3Dot(V1, V2);

	L1 = XMVectorMultiply(L1, L2);

	XMVECTOR CosAngle = XMVectorMultiply(Dot, L1);
	CosAngle = XMVectorClamp(CosAngle, g_XMNegativeOne.v, g_XMOne.v);

	//XMVECTOR angle = XMVector3AngleBetweenVectors(ToCube, CamDir);
	//static const float rads = XMConvertToRadians(40);
	//if (XMVectorGetX(angle) < rads)
	if (XMVectorGetX(CosAngle) > 0.7)
	{
		return true;
	}
	else
	{
		return false;
	}
}

#if 0
bool get_node_mask_at(ecs::system::FrustrumCuller::CullMask* node, const uint16_t index)
{
	const uint16_t idx = (index >> 6);
	const uint16_t shift = (index & 0x3F);
	const uint64_t mask = (uint64_t(0x1) << shift);
	const uint64_t andmask = node->mask[idx] & mask;
	return andmask;
}
void set_node_mask_at(ecs::system::FrustrumCuller::CullMask* node, const uint16_t index)
{
	const uint16_t idx = (index >> 6);
	const uint16_t shift = (index & 0x3F);
	const uint64_t mask = (uint64_t(0x1) << shift);
	node->mask[idx] |= mask;
}
void clear_node_mask_at(ecs::system::FrustrumCuller::CullMask* node, const uint16_t index)
{
	const uint16_t idx = (index >> 6);
	const uint16_t shift = (index & 0x3F);
	const uint64_t mask = (uint64_t(0x1) << shift);
	const uint64_t invmask = uint64_t(-1) ^ mask;
	node->mask[idx] &= invmask;
}

#endif

void AABBFromSpheres(BoundingBox& outbbox, CullSphere* spheres, int Count) {

	// Find the minimum and maximum x, y, and z
	XMVECTOR vMin, vMax;
#if 0
	vMin = vMax = XMLoadFloat4((XMFLOAT4*)&spheres[0].sphere.Center);

	for (size_t i = 1; i < Count; ++i)
	{
		XMVECTOR Point = XMLoadFloat4((XMFLOAT4*)&spheres[i].sphere.Center);

		vMin = XMVectorMin(vMin, Point);
		vMax = XMVectorMax(vMax, Point);
	}
#endif
	vMin = vMax = XMLoadFloat4((XMFLOAT4*)(void*)&spheres[0].sphere.Center);

	for (size_t i = 1; i < Count; ++i)
	{
		XMVECTOR Point = XMLoadFloat4((XMFLOAT4*)(void*)&spheres[i].sphere.Center);

		vMin = XMVectorMin(vMin, Point);
		vMax = XMVectorMax(vMax, Point);
	}
	
	vMin = XMVectorSetW(XMVectorScale(XMVectorAdd(vMin, vMax), 0.5f), 0.f);
	vMax = XMVectorSetW(XMVectorScale(XMVectorSubtract(vMax, vMin), 0.5f), 0.f);

	XMStoreFloat3(&outbbox.Center, XMVectorScale(vMin,0.5));
	XMStoreFloat3(&outbbox.Extents, XMVectorScale(vMax, 0.5));
	// Store center and extents.
	//XMStoreFloat3(&outbbox.Center, XMVectorScale(XMVectorAdd(vMin, vMax), 0.5f));
	//XMStoreFloat3(&outbbox.Extents, XMVectorScale(XMVectorSubtract(vMax, vMin), 0.5f));
}


decs::PureSystemBase* ecs::system::FrustrumCuller::cull_puresys()
{
	static auto puresys = []() {

		Query query;
		query.with<CullSphere, CullBitmask>();
		query.build();

		return decs::make_pure_system_chunk(query, [](void* context, DataChunk* chnk) {

			ECS_GameWorld& world = *reinterpret_cast<ECS_GameWorld*>(context);

			ZoneScopedN("Culling - pure");

			BoundingFrustum tmp(Globals->g_ProjectionMatrix);
			XMVECTOR det;
			BoundingFrustum CameraFrustum;
			tmp.Transform(CameraFrustum, XMMatrixInverse(&det, Globals->g_ViewMatrix));

			XMVECTOR FrustrumPlanes[6];

			CameraFrustum.GetPlanes(
				&FrustrumPlanes[0],
				&FrustrumPlanes[1],
				&FrustrumPlanes[2],
				&FrustrumPlanes[3],
				&FrustrumPlanes[4],
				&FrustrumPlanes[5]
			);
			float zero = 0;
			XMVECTOR zerovec = XMLoadFloat(&zero);

			VisibleRenderChunks* chunkBuffer = world.registry_decs.get_singleton<VisibleRenderChunks>();

			FrustumCullerStructures* cullStructures = world.registry_decs.get_singleton<FrustumCullerStructures>();

			if (!chunkBuffer) { return;
				//chunkBuffer = world.registry_decs.set_singleton<VisibleRenderChunks>();
			}
			if (!cullStructures) { return; 
				//cullStructures = world.registry_decs.set_singleton<FrustumCullerStructures>();
			}

			CulledChunk cullUnit{};
			cullUnit.chunk = chnk;

			auto entities = get_chunk_array<EntityID>(chnk);

			//const bool culled = get_chunk_array<Culled>(chnk).valid();

			auto sphereArray = get_chunk_array<CullSphere>(chnk);
			auto maskArray = get_chunk_array<CullBitmask>(chnk);
			auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);

			//if cullsphere version doesnt match with render matrix version, it needs update
			if (matarray.version() != sphereArray.version())
			{
				ZoneScopedNC("update spheres", tracy::Color::Yellow);
				for (int i = chnk->count() - 1; i >= 0; i--)
				{
					XMVECTOR loc = XMVector3Transform(zerovec, matarray[i].Matrix);

					ecs::system::FrustrumCuller::update_cull_sphere(&sphereArray[i], loc, 10.f);
				}
				world.mark_components_changed(sphereArray);
			}
			BoundingBox bbox;
			DirectX::ContainmentType aabbcont;
			{
				ZoneScopedNC("AABB Build", tracy::Color::Orange);
				AABBFromSpheres(bbox, sphereArray.data, chnk->count());
				aabbcont = bbox.ContainedBy(FrustrumPlanes[0],
					FrustrumPlanes[1],
					FrustrumPlanes[2],
					FrustrumPlanes[3],
					FrustrumPlanes[4],
					FrustrumPlanes[5]);
			}
			{
				ZoneScopedNC("Real Cull", tracy::Color::Red);

				int viscount = 0;
				for (int i = chnk->count() - 1; i >= 0; i--)
				{

					const CullSphere& sphere = sphereArray[i];
					bool bVisible;
					switch (aabbcont) {
					case DirectX::DISJOINT:
						bVisible = false;
						break;
					case DirectX::INTERSECTS:
						bVisible = sphere.sphere.ContainedBy(
							FrustrumPlanes[0],
							FrustrumPlanes[1],
							FrustrumPlanes[2],
							FrustrumPlanes[3],
							FrustrumPlanes[4],
							FrustrumPlanes[5]
						);
						break;
					case DirectX::CONTAINS:
						bVisible = true;
						break;
					}

					if (bVisible) {
						maskArray[i].mask = 1;
						//set_node_mask_at(&cullUnit.mask, i);
						viscount++;
					}
					else {
						maskArray[i].mask = 0;
						//clear_node_mask_at(&cullUnit.mask, i);
					}
				}

				//uint64_t cllversion = maskArray.version();
				maskArray.set_version(viscount);


			}
			
			});


	}();

	return &puresys;
}



void ecs::system::FrustrumCuller::build_view_queues(ECS_GameWorld& world, tf::Subflow& sf)
{
	
		ZoneNamed(FrustrumCuller, true);
		SCOPE_PROFILE("Culling System ")


		BoundingFrustum tmp(Globals->g_ProjectionMatrix);
		XMVECTOR det;
		BoundingFrustum CameraFrustum;
		tmp.Transform(CameraFrustum,XMMatrixInverse(&det,Globals->g_ViewMatrix));
		
		XMVECTOR FrustrumPlanes[6];

		CameraFrustum.GetPlanes(
			&FrustrumPlanes[0],
			&FrustrumPlanes[1],
			&FrustrumPlanes[2],
			&FrustrumPlanes[3],
			&FrustrumPlanes[4],
			&FrustrumPlanes[5]
		);


		VisibleRenderChunks* chunkBuffer = world.registry_decs.get_singleton<VisibleRenderChunks>();

		FrustumCullerStructures* cullStructures = world.registry_decs.get_singleton<FrustumCullerStructures>();

		if (!chunkBuffer) {
			chunkBuffer = world.registry_decs.set_singleton<VisibleRenderChunks>();
		}	
		if (!cullStructures) {
			cullStructures = world.registry_decs.set_singleton<FrustumCullerStructures>();
		}
		
		static std::vector<DataChunk*> chunk_cache;
		chunk_cache.clear();

		Query query;
		query.with<CullSphere, CullBitmask>();
		query.build();
		{
			ZoneScopedNC("Cull Gather Archetypes", tracy::Color::Green);

			world.registry_decs.gather_chunks(query, chunk_cache);
		}

		
		std::atomic_uint32_t push_idx{ 0 };
		float zero = 0;
		XMVECTOR zerovec = XMLoadFloat(&zero);

		parallel_for_chunk(sf,chunk_cache, [&](DataChunk* chnk) {

			ZoneScopedN("Cull Chunk");

			CulledChunk cullUnit{};
			cullUnit.chunk = chnk;

			auto entities = get_chunk_array<EntityID>(chnk);

			//const bool culled = get_chunk_array<Culled>(chnk).valid();

			auto sphereArray = get_chunk_array<CullSphere>(chnk);
			auto maskArray = get_chunk_array<CullBitmask>(chnk);
			auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);

			//if cullsphere version doesnt match with render matrix version, it needs update
			if (matarray.version() != sphereArray.version())
			{
				ZoneScopedNC("update spheres", tracy::Color::Yellow);
				for (int i = chnk->count() - 1; i >= 0; i--)
				{
					XMVECTOR loc = XMVector3Transform(zerovec, matarray[i].Matrix);

					ecs::system::FrustrumCuller::update_cull_sphere(&sphereArray[i], loc, 10.f);
				}
				world.mark_components_changed(sphereArray);
			}
			BoundingBox bbox;
			DirectX::ContainmentType aabbcont;
			{
				ZoneScopedNC("AABB Build", tracy::Color::Orange);
				AABBFromSpheres(bbox, sphereArray.data, chnk->count());
				aabbcont = bbox.ContainedBy(FrustrumPlanes[0],
					FrustrumPlanes[1],
					FrustrumPlanes[2],
					FrustrumPlanes[3],
					FrustrumPlanes[4],
					FrustrumPlanes[5]);
			}
			{
				ZoneScopedNC("Real Cull", tracy::Color::Red);

			int viscount = 0;
			for (int i = chnk->count() - 1; i >= 0; i--)
			{

				const CullSphere& sphere = sphereArray[i];
				bool bVisible;
				switch (aabbcont) {
				case DirectX::DISJOINT:
					bVisible = false;
					break;
				case DirectX::INTERSECTS:
					bVisible = sphere.sphere.ContainedBy(
						FrustrumPlanes[0],
						FrustrumPlanes[1],
						FrustrumPlanes[2],
						FrustrumPlanes[3],
						FrustrumPlanes[4],
						FrustrumPlanes[5]
					);
					break;
				case DirectX::CONTAINS:
					bVisible = true;
					break;
				}

				if (bVisible) {
					maskArray[i].mask = 1;
					//set_node_mask_at(&cullUnit.mask, i);
					viscount++;
				}
				else {
					maskArray[i].mask = 0;
					//clear_node_mask_at(&cullUnit.mask, i);
				}
			}

			//uint64_t cllversion = maskArray.version();
			maskArray.set_version(viscount);

			//if (viscount > 0) {
			//	cullUnit.mask.count = viscount;
			//	ChunkQueue.enqueue(cullUnit);
			//}
			//if (viscount > 0 && culled)
			//{
			//	ChunkChange change;
			//	change.bVisible = true;
			//	change.chunk = chnk;
			//
			//	ChangeChunkQueue.enqueue(change);
			//}
			//else if (viscount == 0 && !culled)
			//{	ChunkChange change;
			//	change.bVisible = false;
			//	change.chunk = chnk;
			//
			//	ChangeChunkQueue.enqueue(change);
			//}

			}
		});
		sf.join();

		//{
		//	ZoneScopedN("Cull Ready");
		//
		//
		//	//chunkBuffer->visibleChunks.clear();
		//
		//	bulk_dequeue(ChangeChunkQueue, [&](ChunkChange& c) {
		//		if (c.bVisible) {
		//			ZoneScopedN("Visible");
		//			decs::adv::remove_component_from_chunk<Culled>(&world.registry_decs, c.chunk);
		//		}
		//		else {
		//			ZoneScopedN("Culled");
		//			decs::adv::add_component_to_chunk<Culled>(&world.registry_decs, c.chunk);
		//		}
		//		});
		//}
}

void ecs::system::FrustrumCuller::apply_queues(ECS_GameWorld& world)
{

	
	return;
	ZoneScopedN("Cull Apply");

	//while (true)
	//{
	//	EntityID results[QueueTraits::BLOCK_SIZE];     // Could also be any iterator
	//	size_t count = SetCulledQueue.try_dequeue_bulk(results, QueueTraits::BLOCK_SIZE);
	//	if (count == 0)break;
	//	for (size_t i = 0; i != count; ++i) {
	//		world.registry_decs.add_component<Culled>(results[i]);
	//	}
	//}
	//while (true)
	//{
	//	EntityID results[QueueTraits::BLOCK_SIZE];     // Could also be any iterator
	//	size_t count = RemoveCulledQueue.try_dequeue_bulk(results, QueueTraits::BLOCK_SIZE);
	//	if (count == 0)break;
	//	for (size_t i = 0; i != count; ++i) {
	//		world.registry_decs.remove_component<Culled>(results[i]);
	//	}
	//}	
}


void ecs::system::FrustrumCuller::update_cull_sphere(CullSphere* sphere, TransformComponent* transform, float scale)
{
	float radius = XMVectorGetX(XMVector3Length(transform->tf->scale)) *scale;

	memcpy(&sphere->sphere, &transform->tf->position, sizeof(XMVECTOR));

	//sphere->sphere.Center = //XMMVector3(// XMVector3  transform->position;
	
	sphere->sphere.Radius = (radius);
}

void ecs::system::FrustrumCuller::update_cull_sphere(CullSphere* sphere, XMVECTOR position, float scale)
{

	memcpy(&sphere->sphere, &position, sizeof(XMVECTOR));

	//sphere->sphere.Center = //XMMVector3(// XMVector3  transform->position;

	sphere->sphere.Radius = (scale);
}

void ecs::system::FrustrumCuller::init_singletons(ECS_GameWorld& world)
{
	VisibleRenderChunks* chunkBuffer = world.registry_decs.get_singleton<VisibleRenderChunks>();
	FrustumCullerStructures* cullStructures = world.registry_decs.get_singleton<FrustumCullerStructures>();

	if (!chunkBuffer) {
		chunkBuffer = world.registry_decs.set_singleton<VisibleRenderChunks>();
	}
	if (!cullStructures) {
		cullStructures = world.registry_decs.set_singleton<FrustumCullerStructures>();
	}
}

void ecs::system::CubeRenderer::pre_render()
{
	
}

struct RendererBuffers {

	struct Page {
		std::vector<XMMATRIX> InstancedTransforms;
		std::vector<XMFLOAT4> InstancedColors;
		std::atomic_uint32_t lenght;
	};

	long FrameIDX;

	Page A;
	Page B;

	Page& getLastPage(){
		if (FrameIDX & 0x1) {
			return A;
		}
		else {
			return B;
		}
	};
	Page& getCurrentPage() {
		if (FrameIDX & 0x1) {
			return B;
		}
		else {
			return A;
		}
	};
};

struct RendererHandles {
	std::unique_ptr<RendererBuffers> CubeBuffers;
};

void ecs::system::CubeRenderer::update(ECS_GameWorld &world)
{
	ZoneNamed(CubeRenderer, true);

	SCOPE_PROFILE("Cube Render System");

	//build_cube_batches(world);	
}
void ecs::system::CubeRenderer::update_par(ECS_GameWorld& world,tf::Subflow& sf)
{
	ZoneNamed(CubeRenderer, true);

	SCOPE_PROFILE("Cube Render System");

	build_cube_batches(world,sf);
}

void ecs::system::CubeRenderer::render_cube_batch(XMMATRIX* FirstMatrix, XMFLOAT4* FirstColor, uint32_t total_drawcalls)
{
	
	const UINT vertexStride = sizeof(VertexPosColor);
	const UINT offset = 0;
	Globals->g_d3dDeviceContext->OMSetRenderTargets(1, &Globals->g_d3dRenderTargetView, Globals->g_d3dDepthStencilView);
	Globals->g_d3dDeviceContext->OMSetDepthStencilState(Globals->g_d3dDepthStencilState, 1);

	Globals->g_d3dDeviceContext->IASetVertexBuffers(0, 1, &Globals->g_d3dVertexBuffer, &vertexStride, &offset);
	Globals->g_d3dDeviceContext->IASetInputLayout(Globals->g_d3dInputLayout);
	Globals->g_d3dDeviceContext->IASetIndexBuffer(Globals->g_d3dIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	Globals->g_d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Globals->g_d3dDeviceContext->VSSetShader(Globals->g_d3dVertexShader, nullptr, 0);

	Globals->g_d3dDeviceContext->VSSetConstantBuffers(0, 3, Globals->g_d3dConstantBuffers);

	Globals->g_d3dDeviceContext->RSSetState(Globals->g_d3dRasterizerState);
	Globals->g_d3dDeviceContext->RSSetViewports(1, &Globals->g_Viewport);

	Globals->g_d3dDeviceContext->PSSetShader(Globals->g_d3dPixelShader, nullptr, 0);

	ZoneScopedN("Render Cube Batches");
	unsigned int bufferidx = 0;
	unsigned int nDrawcalls = 0;
	while (true) {

		uniformBuffer.worldMatrix[bufferidx] = FirstMatrix[nDrawcalls];
		uniformBuffer.color[bufferidx] = FirstColor[nDrawcalls];

		bufferidx++;
		nDrawcalls++;
		if (bufferidx >= 512 || nDrawcalls >= total_drawcalls) 
		{
			ZoneScopedN("Render Upload Batch");

			Globals->g_d3dDeviceContext->UpdateSubresource(Globals->g_d3dConstantBuffers[CB_Object], 0, nullptr, &uniformBuffer, 0, 0);

			Globals->g_d3dDeviceContext->DrawIndexedInstanced(_countof(Globals->g_CubeIndicies), bufferidx, 0, 0, 0);
			bufferidx = 0;
			if (nDrawcalls >= total_drawcalls) {
				break;
			}
		}
	}
}
void ecs::system::CubeRenderer::init_singletons(ECS_GameWorld& world)
{
	RendererHandles* rhandles = world.registry_decs.get_singleton<RendererHandles>();

	if (!rhandles) {
		rhandles = world.registry_decs.set_singleton<RendererHandles>();
		rhandles->CubeBuffers = std::make_unique<RendererBuffers>();
		

		//buffers->InstancedCubes.resize(500000);
	}
	RendererBuffers* buffers = rhandles->CubeBuffers.get();
	buffers->getCurrentPage().lenght.store(0);

	buffers->getCurrentPage().InstancedColors.resize(400000);
	buffers->getCurrentPage().InstancedTransforms.resize(400000);
}
void ecs::system::CubeRenderer::build_cube_batches(ECS_GameWorld& world, tf::Subflow& sf)
{
	RendererHandles* rhandles = world.registry_decs.get_singleton<RendererHandles>();
	RendererBuffers* buffers = rhandles->CubeBuffers.get();
//	buffers->getCurrentPage().lenght.store(0);

	static std::vector<DataChunk*> chunk_cache;
	chunk_cache.clear();

	Query query;
	query.with<CubeRendererComponent, RenderMatrixComponent>();
	//query.exclude<Culled>();
	query.build();

	
	std::atomic_uint32_t total_drawcalls = 0;
	{
		ZoneScopedNC("Render Gather Archetypes", tracy::Color::Green);

		//world.registry_decs.gather_chunks(query, chunk_cache);
		decs::adv::iterate_matching_archetypes(&world.registry_decs, query, [&](Archetype* arch) {

			if (arch->chunks.size() > 0) {
				auto maskArray = get_chunk_array<CullBitmask>(arch->chunks[0]);
				for (auto chnk : arch->chunks) {
					maskArray.chunkOwner = chnk;
					//auto maskArray = get_chunk_array<CullBitmask>(chnk);
					total_drawcalls += maskArray.version();
					chunk_cache.push_back(chnk);
				}
			}
		});
		
		//this is bigger than it should, but it doesnt matter to use the extra memory
		//for (auto chnk : chunkBuffer->visibleChunks) {
		//	total_drawcalls += chnk.mask.count;//chnk->header.last;
		//}
	}

	if (total_drawcalls == 0) return;

	//buffers->lenght.store(total_drawcalls);

	buffers->getCurrentPage().InstancedColors.resize(total_drawcalls);
	buffers->getCurrentPage().InstancedTransforms.resize(total_drawcalls);

	ApplicationInfo* appInfo =world.registry_decs.get_singleton<ApplicationInfo>();
	appInfo->Drawcalls = total_drawcalls.load();

	XMMATRIX* FirstMatrix = &buffers->getCurrentPage().InstancedTransforms[0];
	XMFLOAT4* FirstColor = &buffers->getCurrentPage().InstancedColors[0];

	sf.for_each(chunk_cache.begin(), chunk_cache.end(),
		[&](DataChunk* chnk) {

		ZoneScopedNC("render Execute Chunks", tracy::Color::Red);

		auto matrices = get_chunk_array<RenderMatrixComponent>(chnk);
		auto cubes = get_chunk_array<CubeRendererComponent>(chnk);
		auto maskArray = get_chunk_array<CullBitmask>(chnk);

		if (maskArray.version() > 0) {
			//add atomic to reserve space
			int first_idx = buffers->getCurrentPage().lenght.fetch_add(maskArray.version());
			int n = 0;
			for (unsigned int i = 0; i < chnk->count(); i++)
			{ 
				if (maskArray[i].mask) {
					FirstColor[n + first_idx] = XMFLOAT4(cubes[i].color.x, cubes[i].color.y, cubes[i].color.z, 1.0f);
					FirstMatrix[n + first_idx] = matrices[i].Matrix;
					n++;
				}
			}
		}
	});
	sf.join();
}

decs::PureSystemBase* ecs::system::CubeRenderer::cubebatch_puresys()
{
	static auto puresys = []() {

		Query query;
		query.with<CubeRendererComponent, RenderMatrixComponent>();
		query.build();

		return decs::make_pure_system_chunk(query, [](void* context, DataChunk* chnk) {

			ECS_GameWorld& world = *reinterpret_cast<ECS_GameWorld*>(context);

			ZoneScopedN("Batch Cubes - pure");

			RendererHandles* rhandles = world.registry_decs.get_singleton<RendererHandles>();
			RendererBuffers* buffers = rhandles->CubeBuffers.get();



			ApplicationInfo* appInfo = world.registry_decs.get_singleton<ApplicationInfo>();
			//appInfo->Drawcalls = total_drawcalls.load();

			XMMATRIX* FirstMatrix = &buffers->getCurrentPage().InstancedTransforms[0];
			XMFLOAT4* FirstColor = &buffers->getCurrentPage().InstancedColors[0];

					

					//ZoneScopedNC("render Execute Chunks", tracy::Color::Red);

			auto matrices = get_chunk_array<RenderMatrixComponent>(chnk);
			auto cubes = get_chunk_array<CubeRendererComponent>(chnk);
			auto maskArray = get_chunk_array<CullBitmask>(chnk);

			if (maskArray.version() > 0) {
				//add atomic to reserve space
				int first_idx = buffers->getCurrentPage().lenght.fetch_add(maskArray.version());
				int n = 0;
				for (unsigned int i = 0; i < chnk->count(); i++)
				{
					if (maskArray[i].mask) {
						FirstColor[n + first_idx] = XMFLOAT4(cubes[i].color.x, cubes[i].color.y, cubes[i].color.z, 1.0f);
						FirstMatrix[n + first_idx] = matrices[i].Matrix;
						n++;
					}
				}
			}
		});
	}();

	return &puresys;
}



void ecs::system::RenderCore::render_start()
{
	
	//rmt_BeginD3D11Sample(RenderFrameDX);

	assert(Globals->g_d3dDevice);
	assert(Globals->g_d3dDeviceContext);

	Clear(Colors::DarkBlue, 1.0f, 0);

	drawcalls = 0;
	//ImGui::exam
	static bool bDemoOpen{ false };
	//ImGui::ShowDemoWindow(&bDemoOpen);
	ImGui::Render();
	
}

void ecs::system::RenderCore::Present(bool vSync)
{
	
	
	if (vSync)
	{
		Globals->g_d3dSwapChain->Present(1, 0);
	}
	else
	{
		Globals->g_d3dSwapChain->Present(0, 0);
	}
}

void ecs::system::RenderCore::render_end(ECS_GameWorld& world)
{
	ZoneScopedN("Present");
	{
		//rmt_ScopedD3D11Sample(ImGuiRender);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	} 
	//ImGui::UpdatePlatformWindows();
	//ImGui::RenderPlatformWindowsDefault();
	{
		//rmt_ScopedD3D11Sample(DirectXPresent);
		//Present(Globals->g_EnableVSync);
		Present(false);
	}
	//ImGui::EndFrame();
	

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	RendererHandles* rhandles = world.registry_decs.get_singleton<RendererHandles>();
	if (rhandles) {
		rhandles->CubeBuffers->FrameIDX++;
	}

	drawcalls = nDrawcalls;

	//rmt_EndD3D11Sample();
}


void ecs::system::RenderCore::update(ECS_GameWorld &world)
{
	//render_start();
	//nDrawcalls = 0;
	//
	//cam_updater->update(world);
	//culler->update(world);
	//cube_renderer->update(world);
	//
	//render_end();
}

ecs::system::RenderCore::RenderCore()
{
	cam_updater = new CameraUpdate();
	culler =  new FrustrumCuller();
	cube_renderer = new CubeRenderer();
}

void ecs::system::RenderCore::render_batches(ECS_GameWorld& world)
{
	RendererHandles* rhandles = world.registry_decs.get_singleton<RendererHandles>();
	if (rhandles) {
		if (rhandles->CubeBuffers->getLastPage().InstancedTransforms.size() > 0)
		{
			XMMATRIX* FirstMatrix = &rhandles->CubeBuffers->getLastPage().InstancedTransforms[0];
			XMFLOAT4* FirstColor = &rhandles->CubeBuffers->getLastPage().InstancedColors[0];

			cube_renderer->render_cube_batch(FirstMatrix, FirstColor, rhandles->CubeBuffers->getLastPage().lenght);
		}
	}
}

