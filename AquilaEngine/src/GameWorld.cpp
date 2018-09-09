#include "GameWorld.h"
#include "RandomUtils.h"
#include "ApplicationInfoUI.h"


#include "EngineGlobals.h"
#include "SimpleProfiler.h"
#include "Multivector.h"


#include "Systems/CoreSystems.h"
#include "Systems/BoidSystems.h"
#include "Systems/SpaceshipSystems.h"
#include "Systems/LifetimeSystems.h"
#include "Systems/GameSystems.h"
#include "Systems/TransformSystems.h"
#include "Systems/RenderSystems.h"
void BuildShipSpawner(ECS_Registry & registry, XMVECTOR  Location, XMVECTOR TargetLocation)
{
	auto spawner1 = registry.create();

	float posx = XMVectorGetX(Location) + rng::RandomFloat() * 100;
	float posy = XMVectorGetY(Location) + rng::RandomFloat() * 100 + 200;
	float posz = XMVectorGetZ(Location) + rng::RandomFloat() * 100;

	//registry.assign<PositionComponent>(spawner1, XMFLOAT3(posx, posy, posz));
	SpaceshipSpawnerComponent & spcomp = registry.assign<SpaceshipSpawnerComponent>(spawner1);
	spcomp.Bounds = XMFLOAT3(10, 50, 50);
	spcomp.Elapsed = 0;
	spcomp.SpawnRate = 0.01;
	spcomp.ShipMoveTarget = TargetLocation;

	registry.assign<CubeRendererComponent>(spawner1);
	if (XMVectorGetX(Location) > 0)
	{
		registry.get<CubeRendererComponent>(spawner1).color = XMFLOAT3(1.0f, 0.1f, 0.1f);

	}
	else
	{
		registry.get<CubeRendererComponent>(spawner1).color = XMFLOAT3(0.1f, 0.10f, 1.0f);
	}



	registry.assign<RenderMatrixComponent>(spawner1);
	registry.assign<TransformComponent>(spawner1);
	registry.get<TransformComponent>(spawner1).scale = XMVectorSet(1.0f, 10.0f, 10.0f, 0.0f);
	registry.get<TransformComponent>(spawner1).position = XMVectorSet(posx, posy, posz, 1.0f);

}

ECS_GameWorld::~ECS_GameWorld()
{
	delete Renderer;
	for (auto s : Systems)
	{
		delete s;
	}
}

void ECS_GameWorld::Initialize()
{
	auto cam = registry.create();
	registry.assign<PositionComponent>(cam, XMFLOAT3(0, 0, -100));
	registry.assign<CameraComponent>(cam);
	registry.get<CameraComponent>(cam).focusPoint = XMVectorSet(0, 0, 0, 1);

	registry.assign<ApplicationInfo>(entt::tag_t{}, cam);
	registry.assign<EngineTimeComponent>(entt::tag_t{}, cam);
	registry.assign<RendererRegistryReferenceComponent>(entt::tag_t{}, cam);
	BoidMap * map = new BoidMap();
	registry.assign<BoidReferenceTag>(entt::tag_t{}, cam, map);

	Bench_Start(AllBench);
	ImGui_ImplDX11_NewFrame();


	Systems.push_back(new PlayerInputSystem());
	Systems.push_back(new PlayerCameraSystem());
	Systems.push_back(new RandomFlusherSystem());
	Systems.push_back(new SpaceshipSpawnSystem());
	Systems.push_back(new ExplosionFXSystem());

	Systems.push_back(new BoidHashSystem());
	//Systems.push_back(new ExplosionFXSystem());
	Systems.push_back(new SpaceshipMovementSystem());
	Systems.push_back(new DestructionSystem());
	//Systems.push_back(new DestructionApplySystem());



	Systems.push_back(new RotatorSystem());
	Systems.push_back(new TransformUpdateSystem());


	Renderer = new RenderSystem();

	//auto spawner1 = registry.create();
	ApplicationInfo & appInfo = registry.get<ApplicationInfo>();
	appInfo.averagedDeltaTime = 0.03f;
	appInfo.deltaTime = 0.012343f;
	appInfo.Drawcalls = 10000;
	appInfo.RenderTime = 1.0f;

	for (float z = -1000; z < 1000; z += 100)
	{
		BuildShipSpawner(registry, XMVectorSet(-500, 0, z, 0), XMVectorSet(0, 0, z, 0));
		BuildShipSpawner(registry, XMVectorSet(500, 0, z, 0), XMVectorSet(0, 0, z, 0));
	}
}

void ECS_GameWorld::Update_All(float dt)
{
	ApplicationInfo & appInfo = registry.get<ApplicationInfo>();
	appInfo.TotalEntities = registry.size();
	appInfo.BoidEntities = registry.view<BoidComponent>().size();
	Bench_End(AllBench);
	appInfo.deltaTime = Bench_GetMiliseconds(AllBench);
	Bench_Start(AllBench);
	BenchmarkInfo bench;
	Bench_Start(bench);


	auto & timec = registry.get<EngineTimeComponent>();
	timec.delta_time = dt;


	ecs::Task root_task = task_engine.placeholder();
	ecs::Task end_task = task_engine.placeholder();
	{
		rmt_ScopedCPUSample(ScheduleUpdate, 0);


		root_task.name("Root");

		ecs::Task last_task = root_task;
		ecs::Task latest_task = last_task;
		for (auto s : Systems)
		{
			auto temp = last_task;
			last_task = s->schedule(registry, task_engine, last_task, latest_task);
			latest_task = temp;
		}

		end_task = task_engine.placeholder();
		end_task.gather(last_task);
		end_task.name("End");

	}



	std::ofstream myfile;
	myfile.open("example.txt", std::ios::trunc);
	myfile << task_engine.dump();
	myfile.close();
	{
		rmt_ScopedCPUSample(TaskWait, 0);
		task_engine.wait_for_all();


	}

	{
		rmt_ScopedCPUSample(SimulationUpdate, 0);
		for (auto s : Systems)
		{
			if (!s->uses_threading)
			{
				s->update(registry, dt);
			}
		}
		Bench_End(bench);
		appInfo.SimTime = Bench_GetMiliseconds(bench);
		appInfo.Drawcalls = nDrawcalls;


		debug_elapsed += dt;
		if (debug_elapsed > 1)
		{
			debug_elapsed = 0;
			debugiterations = 0;
			g_SimpleProfiler->displayunits = g_SimpleProfiler->units;
			g_SimpleProfiler->units.clear();
		}
		AppInfoUI(appInfo);
		DrawSystemPerformanceUnits(g_SimpleProfiler->displayunits);


	}
	{
		//rmt_ScopedCPUSample(DeepClone, 0);
		//registry.clone_to(registry);

		//RendererRegistryReferenceComponent & render_reg = registry.get<RendererRegistryReferenceComponent>();
		//render_reg.rg = &render_registry;

	}

	{
		rmt_ScopedCPUSample(RenderUpdate, 0);
		Bench_Start(bench);
		Renderer->update(registry, dt);
		Bench_End(bench);
		appInfo.RenderTime = Bench_GetMiliseconds(bench);

	}
	for (auto s : Systems)
	{
		s->cleanup(registry);
	}
}
