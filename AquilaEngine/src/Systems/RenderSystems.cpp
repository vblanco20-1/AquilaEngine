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
	rmt_ScopedCPUSample(CameraUpdate, 0);

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
	update(world.registry_entt, world.GetTime().delta_time);
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
	rmt_ScopedCPUSample(FrustrumCuller, 0);
	SCOPE_PROFILE("Culling System ")
		XMVECTOR CamPos;
	XMVECTOR CamDir;

	world.registry_entt.view<PositionComponent, CameraComponent>(/*entt::persistent_t{}*/).each([&](auto entity, PositionComponent & campos, CameraComponent & cam) {

		//XMFLOAT3::
		CamPos = XMLoadFloat3(&campos.Position);
		//XMFLOAT3 FocalPoint{ XMVectorGetX(cam.focusPoint),XMVectorGetY(cam.focusPoint),XMVectorGetZ(cam.focusPoint) };
		CamDir = XMLoadFloat3(&campos.Position) - cam.focusPoint;
	});

	CamDir = XMVector3Normalize(CamDir);

	Archetype RenderTuple;
	RenderTuple.AddComponent<RenderMatrixComponent>();
	RenderTuple.AddComponent<CubeRendererComponent>();

	Archetype CullTuple;
	CullTuple.AddComponent<Culled>();

	//auto  posview = registry.view<RenderMatrixComponent, CubeRendererComponent>(entt::persistent_t{});

	//iterate nonculled blocks

	moodycamel::ConcurrentQueue<EntityHandle> SetCulledQueue;
	moodycamel::ConcurrentQueue<EntityHandle> RemoveCulledQueue;

	world.registry_decs.IterateBlocks(RenderTuple.componentlist, [&](ArchetypeBlock & block) {
		const bool bHasCull = block.myArch.Match(CullTuple.componentlist) == 1;
		
		auto cubearray = block.GetComponentArray<CubeRendererComponent>();
		auto transfarray = block.GetComponentArray<RenderMatrixComponent>();
		for (int i = block.last-1; i >= 0; i--)
		{
			const CubeRendererComponent & cube = cubearray.Get(i);
			const RenderMatrixComponent & matrix = transfarray.Get(i);
			bool bVisible = IsVisibleFrustrumCull(matrix, cube, CamPos, CamDir);

			if (bHasCull && bVisible)
			{
				RemoveCulledQueue.enqueue(block.entities[i]);
			}
			else if (!bHasCull && !bVisible)
			{
				SetCulledQueue.enqueue(block.entities[i]);
			}
			
		}
	}, true);	

	EntityHandle handle;
	while (SetCulledQueue.try_dequeue(handle)) {

		world.registry_decs.AddComponent<Culled>(handle);
	}
	while (RemoveCulledQueue.try_dequeue(handle)) {

		world.registry_decs.RemoveComponent<Culled>(handle);
	}

	////XMVECTOR VecDir = 
	//std::for_each(std::execution::par_unseq, posview.begin(), posview.end(), [&](const auto entity) {
	//
	//
	//	auto[matrix, cube] = posview.get<RenderMatrixComponent, CubeRendererComponent>(entity);
	//	//posview.each([&, dt](auto entity, RenderMatrixComponent & matrix, PositionComponent&posc,CubeRendererComponent &cube) {
	//
	//	
	//});





	//update(world.registry_entt, world.GetTime().delta_time);
}



void ecs::system::CubeRenderer::pre_render()
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
}

void ecs::system::CubeRenderer::update(ECS_Registry &registry, float dt)
{
	rmt_ScopedCPUSample(CubeRenderer, 0);
	rmt_ScopedD3D11Sample(CubeRendererSystemDX);

	SCOPE_PROFILE("Cube Render System");
	//auto p = ScopeProfiler("Cube Render System", *g_SimpleProfiler);


	pre_render();

	int bufferidx = 0;
	rmt_ScopedD3D11Sample(IterateCubes);

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
	rmt_ScopedCPUSample(CubeRenderer, 0);
	rmt_ScopedD3D11Sample(CubeRendererSystemDX);

	SCOPE_PROFILE("Cube Render System");
	//auto p = ScopeProfiler("Cube Render System", *g_SimpleProfiler);


	pre_render();

	int bufferidx = 0;
	rmt_ScopedD3D11Sample(IterateCubes);

	//rmt_BeginD3D11Sample(RenderCubeBatch);
	Archetype RenderTuple;
	RenderTuple.AddComponent<RenderMatrixComponent>();
	RenderTuple.AddComponent<CubeRendererComponent>();


	Archetype CullTuple;
	CullTuple.AddComponent<Culled>();

	world.registry_decs.IterateBlocks(RenderTuple.componentlist, CullTuple.componentlist,[&](ArchetypeBlock & block) {
	
		auto cubearray = block.GetComponentArray<CubeRendererComponent>();
		auto transfarray = block.GetComponentArray<RenderMatrixComponent>();
		for (int i = 0; i < block.last; i++)
		{
			const CubeRendererComponent & cube = cubearray.Get(i);
			const RenderMatrixComponent & matrix = transfarray.Get(i);
			//if (cube.bVisible)
			//{
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
			//}
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


void ecs::system::RenderCore::render_start()
{
	rmt_BeginD3D11Sample(RenderFrameDX);

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
	{
		rmt_ScopedD3D11Sample(ImGuiRender);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	{
		rmt_ScopedD3D11Sample(DirectXPresent);
		Present(Globals->g_EnableVSync);
	}

	ImGui_ImplDX11_NewFrame();

	drawcalls = nDrawcalls;

	rmt_EndD3D11Sample();
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

	for (auto s : Renderers)
	{
		s->update(world);
	}

	render_end();
}

ecs::system::RenderCore::RenderCore()
{
	Renderers.push_back(new CameraUpdate());
	Renderers.push_back(new FrustrumCuller());
	Renderers.push_back(new CubeRenderer());
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
		for (auto s : Renderers)
		{
			last_task = s->schedule(registry, task_engine, last_task, last_task);
		}


		end_task.gather(last_task);
		end_task.name("Render End");

	}


	return std::move(end_task);
}
