#pragma once
#include <PrecompiledHeader.h>
#include "DXShaders.h"
// Clear the color and depth buffers.
void Clear(const FLOAT clearColor[4], FLOAT clearDepth, UINT8 clearStencil)
{
	Globals->g_d3dDeviceContext->ClearRenderTargetView(Globals->g_d3dRenderTargetView, clearColor);
	Globals->g_d3dDeviceContext->ClearDepthStencilView(Globals->g_d3dDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, clearStencil);
}



static int nDrawcalls;

struct CubeRendererSystem : public System {
	ObjectUniformStruct uniformBuffer;

	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

		ecs::Task task = task_engine.placeholder();
		task.name("Cube Renderer System");
		//run after the parent
		task.gather(parent);
		return std::move(task);
	};


	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(CubeRendererSystem, 0);
		rmt_ScopedD3D11Sample(CubeRendererSystemDX);

		SCOPE_PROFILE("Cube Render System");
		//auto p = ScopeProfiler("Cube Render System", *g_SimpleProfiler);
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



	};
};

struct RenderSystem : public System {

	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

		ecs::Task root_task = task_engine.placeholder();
		root_task.name("Render Start");
		//run after the parent
		root_task.gather(parent);


		ecs::Task end_task = task_engine.placeholder();

		{
			//rmt_ScopedCPUSample(ScheduleUpdate, 0);


			//root_task.name("Root");

			ecs::Task last_task = root_task;
			for (auto s : Renderers)
			{
				last_task = s->schedule(registry, task_engine, last_task, last_task);
			}


			end_task.gather(last_task);
			end_task.name("Render End");

		}


		return std::move(end_task);
	};


	RenderSystem()
	{
		Renderers.push_back(new CameraSystem());
		Renderers.push_back(new CullingSystem());
		Renderers.push_back(new CubeRendererSystem());
	}
	virtual void update(ECS_Registry &registry, float dt)
	{
		render_start();
		nDrawcalls = 0;

		for (auto s : Renderers)
		{
			s->update(registry, dt);
		}

		render_end();
	}

	void render_start() {

		rmt_BeginD3D11Sample(RenderFrameDX);

		assert(Globals->g_d3dDevice);
		assert(Globals->g_d3dDeviceContext);

		Clear(Colors::DarkBlue, 1.0f, 0);


		//ImGui::exam
		static bool bDemoOpen{ false };
		//ImGui::ShowDemoWindow(&bDemoOpen);
		ImGui::Render();
	}
	void render_end() {

		{
			rmt_ScopedD3D11Sample(ImGuiRender);

			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}
		{
			rmt_ScopedD3D11Sample(DirectXPresent);
			Present(Globals->g_EnableVSync);
		}

		ImGui_ImplDX11_NewFrame();

		rmt_EndD3D11Sample();
	}
	void Present(bool vSync)
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

	std::vector<System*> Renderers;

};
