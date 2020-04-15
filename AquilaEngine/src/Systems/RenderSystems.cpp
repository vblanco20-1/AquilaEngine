#include "Systems/RenderSystems.h"
#include "GameWorld.h"


// Clear the color and depth buffers.
void Clear(const FLOAT clearColor[4], FLOAT clearDepth, UINT8 clearStencil)
{
	Globals->g_d3dDeviceContext->ClearRenderTargetView(Globals->g_d3dRenderTargetView, clearColor);
	Globals->g_d3dDeviceContext->ClearDepthStencilView(Globals->g_d3dDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, clearStencil);
}



void ecs::system::CameraUpdate::update(ECS_Registry &registry, float dt)
{
	ZoneNamed(CameraUpdate, true);

	registry.view<PositionComponent, CameraComponent>(/*entt::persistent_t{}*/).each([&, dt](auto entity, PositionComponent & campos, CameraComponent & cam) {


		XMVECTOR eyePosition = XMVectorSet(campos.Position.x, campos.Position.y, campos.Position.z, 1);; //XMVectorSet(0, 0, -70, 1);
		XMVECTOR focusPoint = cam.focusPoint;// + XMVectorSet(CamOffset.x, CamOffset.y, 0.0f, 0.0f); //XMVectorSet(0, 0, 0, 1);
		XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);

		Globals->g_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
		Globals->g_d3dDeviceContext->UpdateSubresource(Globals->g_d3dConstantBuffers[CB_Frame], 0, nullptr, &Globals->g_ViewMatrix, 0, 0);

		//rotation.Angle += 90.0f * dt;
	});
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
	XMVECTOR angle = XMVector3AngleBetweenVectors(ToCube, CamDir);
	static const float rads = XMConvertToRadians(40);
	if (XMVectorGetX(angle) < rads)
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

		static std::vector<DataChunk*> chunk_cache;
		chunk_cache.clear();

		Query query;
		query.with<CubeRendererComponent, RenderMatrixComponent>();
		query.build();
		{
			ZoneScopedNC("Cull Gather Archetypes", tracy::Color::Green);

			world.registry_decs.gather_chunks(query, chunk_cache);
		}
		std::for_each(std::execution::par, chunk_cache.begin(), chunk_cache.end(), [&](DataChunk* chnk) {
			chull_chunk(chnk, CamPos, CamDir);

		});

	
}

void ecs::system::FrustrumCuller::apply_queues(ECS_GameWorld& world)
{
	
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
	nDrawcalls = 0;
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
}

void ecs::system::CubeRenderer::update(ECS_Registry &registry, float dt)
{
	ZoneNamed(CubeRenderer, true);
	//rmt_ScopedD3D11Sample(CubeRendererSystemDX);

	SCOPE_PROFILE("Cube Render System");
	//auto p = ScopeProfiler("Cube Render System", *g_SimpleProfiler);


	pre_render();

	int bufferidx = 0;
	//rmt_ScopedD3D11Sample(IterateCubes);

	//rmt_BeginD3D11Sample(RenderCubeBatch);

	registry.view<RenderMatrixComponent, CubeRendererComponent>().each([&, dt](auto entity, RenderMatrixComponent & matrix, CubeRendererComponent & cube) {
		if (cube.bVisible)
		{
			nDrawcalls++;

			////ObjectUniformStruct uniform;
			//uniform.worldMatrix = matrix.Matrix;
			//uniform.color = XMFLOAT4( cube.color.x, cube.color.y, cube.color.z,1.0f) ;

			uniformBuffer.worldMatrix[bufferidx] = matrix.Matrix;
			uniformBuffer.color[bufferidx] = XMFLOAT4(cube.color.x, cube.color.y, cube.color.z, 1.0f);
			bufferidx++;
			if (bufferidx >= 512)
			{
				bufferidx = 0;
				Globals->g_d3dDeviceContext->UpdateSubresource(Globals->g_d3dConstantBuffers[CB_Object], 0, nullptr, &uniformBuffer, 0, 0);

				Globals->g_d3dDeviceContext->DrawIndexedInstanced(_countof(Globals->g_CubeIndicies), 512, 0, 0, 0);

				//rmt_EndD3D11Sample();
				//rmt_BeginD3D11Sample(RenderCubeBatch);
			}

			//Globals->g_d3dDeviceContext->DrawIndexed(_countof(Globals->g_CubeIndicies), 0, 0);
		}
	});

	if (bufferidx > 0)
	{
		Globals->g_d3dDeviceContext->UpdateSubresource(Globals->g_d3dConstantBuffers[CB_Object], 0, nullptr, &uniformBuffer, 0, 0);

		Globals->g_d3dDeviceContext->DrawIndexedInstanced(_countof(Globals->g_CubeIndicies), bufferidx, 0, 0, 0);
		//rmt_EndD3D11Sample();
		//rmt_BeginD3D11Sample(RenderCubeBatch);
	}
}

void ecs::system::CubeRenderer::update(ECS_GameWorld &world)
{
	ZoneNamed(CubeRenderer, true);
	//rmt_ScopedD3D11Sample(CubeRendererSystemDX);

	SCOPE_PROFILE("Cube Render System");
	//auto p = ScopeProfiler("Cube Render System", *g_SimpleProfiler);


	pre_render();

	int bufferidx = 0;
	//rmt_ScopedD3D11Sample(IterateCubes);
	
	auto* reg = &world.registry_decs;
	float dt = world.GetTime().delta_time;

	Query query;
	query.with<CubeRendererComponent, RenderMatrixComponent>();
	query.exclude<Culled>();
	query.build();	

	reg->for_each(query,[&](RenderMatrixComponent& matrix, CubeRendererComponent& cube) {
		
		nDrawcalls++;

		uniformBuffer.worldMatrix[bufferidx] = matrix.Matrix;
		uniformBuffer.color[bufferidx] = XMFLOAT4(cube.color.x, cube.color.y, cube.color.z, 1.0f);
		bufferidx++;
		if (bufferidx >= 512)
		{
			ZoneScopedN("Render Cube Batch");
			bufferidx = 0;
			Globals->g_d3dDeviceContext->UpdateSubresource(Globals->g_d3dConstantBuffers[CB_Object], 0, nullptr, &uniformBuffer, 0, 0);

			Globals->g_d3dDeviceContext->DrawIndexedInstanced(_countof(Globals->g_CubeIndicies), 512, 0, 0, 0);
		}
	});


	if (bufferidx > 0)
	{
		ZoneScopedN("Render Cube Batch");
		
		Globals->g_d3dDeviceContext->UpdateSubresource(Globals->g_d3dConstantBuffers[CB_Object], 0, nullptr, &uniformBuffer, 0, 0);

		Globals->g_d3dDeviceContext->DrawIndexedInstanced(_countof(Globals->g_CubeIndicies), bufferidx, 0, 0, 0);		
	}
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
	
	FrameMark;
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
		Present(true);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	drawcalls = nDrawcalls;

	//rmt_EndD3D11Sample();
}

void ecs::system::RenderCore::update(ECS_Registry &registry, float dt)
{
	//render_start();
	//nDrawcalls = 0;
	//
	//for (auto s : Renderers)
	//{
	//	s->update(registry, dt);
	//}
	//
	//render_end();
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

ecs::Task ecs::system::RenderCore::schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent)
{
	ecs::Task root_task = task_engine.placeholder();
	root_task.name("Render Start");
	//run after the parent
	root_task.gather(parent);


	ecs::Task end_task = task_engine.placeholder();
	{


		ecs::Task last_task = root_task;
		//for (auto s : Renderers)
		//{
		//	last_task = s->schedule(registry, task_engine, last_task, last_task);
		//}


		end_task.gather(last_task);
		end_task.name("Render End");

	}


	return std::move(end_task);
}
