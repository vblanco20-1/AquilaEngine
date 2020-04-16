#include "Systems/RenderSystems.h"
#include "GameWorld.h"


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

bool IsVisibleFrustrumCull(const RenderMatrixComponent & matrix, const CubeRendererComponent & cube,const XMVECTOR &CamPos,const XMVECTOR& CamDir ) {
	XMVECTOR pos = XMVector3Transform(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), matrix.Matrix);
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

void ecs::system::FrustrumCuller::update(ECS_GameWorld &world)
{
	build_view_queues(world);
	apply_queues(world);
}



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


void ecs::system::FrustrumCuller::build_view_queues(ECS_GameWorld& world)
{
	
		ZoneNamed(FrustrumCuller, true);
		SCOPE_PROFILE("Culling System ")
			XMVECTOR CamPos;
		XMVECTOR CamDir;
		world.registry_decs.for_each([&](EntityID entity, PositionComponent& campos, CameraComponent& cam) {
			
			CamPos = XMLoadFloat3(&campos.Position);
			
			CamDir = XMLoadFloat3(&campos.Position) - cam.focusPoint;
		});

		CamDir = XMVector3Normalize(CamDir);


		VisibleRenderChunks* chunkBuffer = world.registry_decs.get_singleton<VisibleRenderChunks>();

		if (!chunkBuffer) {
			chunkBuffer = world.registry_decs.set_singleton<VisibleRenderChunks>();
		}		
		
		static std::vector<DataChunk*> chunk_cache;
		chunk_cache.clear();

		Query query;
		query.with<CubeRendererComponent, RenderMatrixComponent>();
		query.build();
		{
			ZoneScopedNC("Cull Gather Archetypes", tracy::Color::Green);

			world.registry_decs.gather_chunks(query, chunk_cache);
		}

		
		std::atomic_uint32_t push_idx{ 0 };

		parallel_for_chunk(chunk_cache, [&](DataChunk* chnk) {
		

			ZoneScopedN("Cull Chunk");

			CulledChunk cullUnit{};
			cullUnit.chunk = chnk;
			

			auto entities = get_chunk_array<EntityID>(chnk);
			auto cubearray = get_chunk_array<CubeRendererComponent>(chnk);
			auto transfarray = get_chunk_array<RenderMatrixComponent>(chnk);	

			//bool any_visible = false;
			int viscount = 0;
			for (int i = chnk->header.last - 1; i >= 0; i--)
			{
				const CubeRendererComponent& cube = cubearray[i];
				const RenderMatrixComponent& matrix = transfarray[i];
				bool bVisible = IsVisibleFrustrumCull(matrix, cube, CamPos, CamDir);
				
				if (bVisible) {
					set_node_mask_at(&cullUnit.mask, i);
					viscount++;
				}
				else {
					clear_node_mask_at(&cullUnit.mask, i);
				}
			}

			if (viscount > 0) {
				cullUnit.mask.count = viscount;
				ChunkQueue.enqueue(cullUnit);
				//int idx = push_idx.fetch_add(1);
				//
				//chunkBuffer->masks[idx] = mask;
				//chunkBuffer->visibleChunks[idx] = chnk;
			}
		});

		{
			ZoneScopedN("Cull Ready");

			chunkBuffer->visibleChunks.clear();
			//chunkBuffer->masks.clear();

			bulk_dequeue(ChunkQueue, [&](CulledChunk& c) {
				chunkBuffer->visibleChunks.push_back(c);
				//chunkBuffer->masks.push_back(c.mask);
			});
		}
}

void ecs::system::FrustrumCuller::apply_queues(ECS_GameWorld& world)
{
	return;
	ZoneScopedN("Cull Apply", true);

	while (true)
	{
		EntityID results[QueueTraits::BLOCK_SIZE];     // Could also be any iterator
		size_t count = SetCulledQueue.try_dequeue_bulk(results, QueueTraits::BLOCK_SIZE);
		if (count == 0)break;
		for (size_t i = 0; i != count; ++i) {
			world.registry_decs.add_component<Culled>(results[i]);
		}
	}
	while (true)
	{
		EntityID results[QueueTraits::BLOCK_SIZE];     // Could also be any iterator
		size_t count = RemoveCulledQueue.try_dequeue_bulk(results, QueueTraits::BLOCK_SIZE);
		if (count == 0)break;
		for (size_t i = 0; i != count; ++i) {
			world.registry_decs.remove_component<Culled>(results[i]);
		}
	}	
}

void ecs::system::FrustrumCuller::chull_chunk(DataChunk* chnk, XMVECTOR CamPos, XMVECTOR CamDir)
{
	ZoneScopedN("Cull Chunk");
	auto entities = get_chunk_array<EntityID>(chnk);
	auto cubearray = get_chunk_array<CubeRendererComponent>(chnk);
	auto transfarray = get_chunk_array<RenderMatrixComponent>(chnk);
	auto cullarray = get_chunk_array<Culled>(chnk);

	const bool bHasCull = cullarray.valid();

	for (int i = chnk->header.last - 1; i >= 0; i--)
	{
		const CubeRendererComponent& cube = cubearray[i];
		const RenderMatrixComponent& matrix = transfarray[i];
		bool bVisible = IsVisibleFrustrumCull(matrix, cube, CamPos, CamDir);

		if (bHasCull && bVisible)
		{
			RemoveCulledQueue.enqueue(entities[i]);
		}
		else if (!bHasCull && !bVisible)
		{
			SetCulledQueue.enqueue(entities[i]);
		}
	}
}

void ecs::system::CubeRenderer::pre_render()
{
	
}

struct RendererBuffers {

	std::vector<XMMATRIX> InstancedTransforms;
	std::vector<XMFLOAT4> InstancedColors;
	std::atomic_uint32_t lenght;
};

struct RendererHandles {
	std::unique_ptr<RendererBuffers> CubeBuffers;
};

void ecs::system::CubeRenderer::update(ECS_GameWorld &world)
{
	ZoneNamed(CubeRenderer, true);

	SCOPE_PROFILE("Cube Render System");

	build_cube_batches(world);	
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
	int bufferidx = 0;
	int nDrawcalls = 0;
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

void ecs::system::CubeRenderer::build_cube_batches(ECS_GameWorld& world)
{
	RendererHandles* rhandles = world.registry_decs.get_singleton<RendererHandles>();

	if (!rhandles) {
		rhandles = world.registry_decs.set_singleton<RendererHandles>();
		rhandles->CubeBuffers = std::make_unique<RendererBuffers>();
		//buffers->InstancedCubes.resize(500000);
	}

	RendererBuffers* buffers = rhandles->CubeBuffers.get();
	buffers->lenght.store(0);

	ecs::system::FrustrumCuller::VisibleRenderChunks* chunkBuffer = world.registry_decs.get_singleton<ecs::system::FrustrumCuller::VisibleRenderChunks>();
	if (!chunkBuffer) return;

	static std::vector<DataChunk*> chunk_cache;
	chunk_cache.clear();

	Query query;
	query.with<CubeRendererComponent, RenderMatrixComponent>();
	//query.exclude<Culled>();
	query.build();

	
	std::atomic_uint32_t total_drawcalls = 0;
	{
		ZoneScopedNC("Render Gather Archetypes", tracy::Color::Green);

		world.registry_decs.gather_chunks(query, chunk_cache);
		//decs::adv::iterate_matching_archetypes(&world.registry_decs, query, [&](Archetype* arch) {

		//	for (auto chnk : arch->chunks) {
		//		total_drawcalls += chnk->header.last;
		//	}
		//});
		
		//this is bigger than it should, but it doesnt matter to use the extra memory
		for (auto chnk : chunkBuffer->visibleChunks) {
			total_drawcalls += chnk.mask.count;//chnk->header.last;
		}
	}


	//buffers->lenght.store(total_drawcalls);

	buffers->InstancedColors.resize(total_drawcalls);
	buffers->InstancedTransforms.resize(total_drawcalls);


	XMMATRIX* FirstMatrix = &buffers->InstancedTransforms[0];
	XMFLOAT4* FirstColor = &buffers->InstancedColors[0];
	std::for_each(std::execution::par, chunkBuffer->visibleChunks.begin(), chunkBuffer->visibleChunks.end(),
		[&](ecs::system::FrustrumCuller::CulledChunk& chnk) {

		ZoneScopedNC("render Execute Chunks", tracy::Color::Red);

		auto matrices = get_chunk_array<RenderMatrixComponent>(chnk.chunk);
		auto cubes = get_chunk_array<CubeRendererComponent>(chnk.chunk);

		//add atomic to reserve space
		int first_idx = buffers->lenght.fetch_add(chnk.mask.count);
		int n = 0;
		for (int i = 0; i < chnk.chunk->header.last; i++)
		{
			if (get_node_mask_at(&chnk.mask, i)) {
				FirstColor[n + first_idx] = XMFLOAT4(cubes[i].color.x, cubes[i].color.y, cubes[i].color.z, 1.0f);
				FirstMatrix[n + first_idx] = matrices[i].Matrix;
				n++;
			}			
		}
	});
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

void ecs::system::RenderCore::render_end()
{
	ZoneScopedN("Present");
	{
		//rmt_ScopedD3D11Sample(ImGuiRender);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	{
		//rmt_ScopedD3D11Sample(DirectXPresent);
		//Present(Globals->g_EnableVSync);
		Present(false);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	drawcalls = nDrawcalls;

	//rmt_EndD3D11Sample();
}


void ecs::system::RenderCore::update(ECS_GameWorld &world)
{
	render_start();
	nDrawcalls = 0;

	cam_updater->update(world);
	culler->update(world);
	cube_renderer->update(world);

	render_end();
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
		XMMATRIX* FirstMatrix = &rhandles->CubeBuffers->InstancedTransforms[0];
		XMFLOAT4* FirstColor = &rhandles->CubeBuffers->InstancedColors[0];

		cube_renderer->render_cube_batch(FirstMatrix, FirstColor, rhandles->CubeBuffers->lenght);

	}
}

