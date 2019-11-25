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

	float  randomtint = rng::RandomFloat();
	if (XMVectorGetX(Location) > 0)
	{
		registry.get<CubeRendererComponent>(spawner1).color = XMFLOAT3(1.0f, randomtint, randomtint);

	}
	else
	{
		registry.get<CubeRendererComponent>(spawner1).color = XMFLOAT3(randomtint, randomtint, 1.0f);
	}



	registry.assign<RenderMatrixComponent>(spawner1);
	registry.assign<TransformComponent>(spawner1);
	registry.get<TransformComponent>(spawner1).scale = XMVectorSet(1.0f, 10.0f, 10.0f, 0.0f);
	registry.get<TransformComponent>(spawner1).position = XMVectorSet(posx, posy, posz, 1.0f);


	ecs::system::UpdateTransform::build_matrix(
		registry.get<TransformComponent>(spawner1), 
		registry.get<RenderMatrixComponent>(spawner1));
}

void BuildShipSpawner(ECS_GameWorld& world, XMVECTOR  Location, XMVECTOR TargetLocation)
{
	//Archetype ARC_ShipSpawner;
	//ARC_ShipSpawner.add_component<SpaceshipSpawnerComponent>();
	//ARC_ShipSpawner.add_component<RenderMatrixComponent>();
	//ARC_ShipSpawner.add_component<TransformComponent>();
	//ARC_ShipSpawner.add_component<CubeRendererComponent>();
	//ARC_ShipSpawner.add_component<RotatorComponent>();

	const Metatype* typesspawner[] =
	{
			get_metatype<SpaceshipSpawnerComponent>(),
			get_metatype<RenderMatrixComponent>(),
			get_metatype<TransformComponent>(),
			get_metatype<CubeRendererComponent>(),
			get_metatype<RotatorComponent>()
	};

	auto* reg = &world.registry_decs;
	Archetype* spawnerArchetype = find_or_create_archetype(reg, typesspawner, 5);

	auto spawner1 = create_entity_with_archetype(spawnerArchetype);//registry.CreateEntity(ARC_ShipSpawner);

	float posx = XMVectorGetX(Location) + rng::RandomFloat() * 200;
	float posy = XMVectorGetY(Location) + rng::RandomFloat() * 200 + 200;
	float posz = XMVectorGetZ(Location) + rng::RandomFloat() * 200;

	SpaceshipSpawnerComponent  spcomp;
	spcomp.Bounds = XMFLOAT3(10, 50, 50);
	spcomp.Elapsed = 0;
	spcomp.SpawnRate = 0.01;
	spcomp.ShipMoveTarget = TargetLocation;
	get_component<SpaceshipSpawnerComponent>(reg,spawner1) = spcomp;
	
	get_component<RotatorComponent>(reg,spawner1).Axis = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	get_component<RotatorComponent>(reg,spawner1).rate = 1;
	float  randomtint = rng::RandomFloat();
	if (XMVectorGetX(Location) > 0)
	{
		get_component<CubeRendererComponent>(reg, spawner1).color = XMFLOAT3(1.0f, randomtint, randomtint);
	
	}
	else
	{
		get_component<CubeRendererComponent>(reg, spawner1).color = XMFLOAT3(randomtint, randomtint, 1.0f);
	}

	get_component<TransformComponent>(reg,spawner1).scale = XMVectorSet(1.0f, 10.0f, 10.0f, 0.0f);
	get_component<TransformComponent>(reg,spawner1).position = XMVectorSet(posx, posy, posz, 1.0f);
	get_component<TransformComponent>(reg,spawner1).rotationQuat = XMQuaternionIdentity();

	ecs::system::UpdateTransform::build_matrix(get_component<TransformComponent>(reg, spawner1), get_component<RenderMatrixComponent>(reg, spawner1));


	//registry.assign<PositionComponent>(spawner1, XMFLOAT3(posx, posy, posz));
	//registry.add_component<SpaceshipSpawnerComponent>(spawner1);//registry.assign<SpaceshipSpawnerComponent>(spawner1);
	//SpaceshipSpawnerComponent  spcomp;
	//spcomp.Bounds = XMFLOAT3(10, 50, 50);
	//spcomp.Elapsed = 0;
	//spcomp.SpawnRate = 0.01;
	//spcomp.ShipMoveTarget = TargetLocation;
	//registry.GetComponent<SpaceshipSpawnerComponent>(spawner1) = spcomp;
	//
	//registry.GetComponent<RotatorComponent>(spawner1).Axis = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	//registry.GetComponent<RotatorComponent>(spawner1).rate = 1;
	//float  randomtint = rng::RandomFloat();
	//if (XMVectorGetX(Location) > 0)
	//{
	//	registry.get<CubeRendererComponent>(spawner1).color = XMFLOAT3(1.0f, randomtint, randomtint);
	//
	//}
	//else
	//{
	//	registry.get<CubeRendererComponent>(spawner1).color = XMFLOAT3(randomtint, randomtint, 1.0f);
	//}


	//registry.get<TransformComponent>(spawner1).scale = XMVectorSet(1.0f, 10.0f, 10.0f, 0.0f);
	//registry.get<TransformComponent>(spawner1).position = XMVectorSet(posx, posy, posz, 1.0f);
	//registry.get<TransformComponent>(spawner1).rotationQuat = XMQuaternionIdentity();
	//
	//ecs::system::UpdateTransform::build_matrix(registry.get<TransformComponent>(spawner1), registry.get<RenderMatrixComponent>(spawner1));
}

ECS_GameWorld::~ECS_GameWorld()
{
	delete Renderer;
	for (auto s : Systems)
	{
		delete s;
	}
}

void ECS_GameWorld::initialize()
{

	//registry_decs.BlockStorage.reserve(10000);
	auto cam = registry_entt.create();
	registry_entt.assign<PositionComponent>(cam, XMFLOAT3(0, 600, 2000));
	registry_entt.assign<CameraComponent>(cam);
	registry_entt.get<CameraComponent>(cam).focusPoint = XMVectorSet(0, 0, 0, 1);

	registry_entt.assign<ApplicationInfo>(entt::tag_t{}, cam);
	registry_entt.assign<EngineTimeComponent>(entt::tag_t{}, cam);
	registry_entt.assign<RendererRegistryReferenceComponent>(entt::tag_t{}, cam);
	BoidMap * map = new BoidMap();
	registry_entt.assign<BoidReferenceTag>(entt::tag_t{}, cam, map);

	Bench_Start(AllBench);
	ImGui_ImplDX11_NewFrame();


	Systems.push_back(new PlayerInputSystem());
	Systems.push_back(new PlayerCameraSystem());
	//Systems.push_back(new RandomFlusherSystem());
	Systems.push_back(new SpaceshipSpawnSystem());
	//Systems.push_back(new ExplosionFXSystem());
	//
	Systems.push_back(new BoidHashSystem());
	////Systems.push_back(new ExplosionFXSystem());
	Systems.push_back(new SpaceshipMovementSystem());
	Systems.push_back(new DestructionSystem());
	////Systems.push_back(new DestructionApplySystem());
	//
	//
	//
	Systems.push_back(new RotatorSystem());
	Systems.push_back(new ecs::system::UpdateTransform());


	Renderer = new ecs::system::RenderCore();

	//auto spawner1 = registry.create();
	ApplicationInfo & appInfo = registry_entt.get<ApplicationInfo>();
	appInfo.averagedDeltaTime = 0.03f;
	appInfo.deltaTime = 0.012343f;
	appInfo.Drawcalls = 10000;
	appInfo.RenderTime = 1.0f;

	for (float z = -1000.0; z < 1000.0; z += 10)
	{
		BuildShipSpawner(*this, XMVectorSet(-500, 0, z, 0), XMVectorSet(0, 0, z, 0));
		BuildShipSpawner(*this, XMVectorSet(500, 0, z, 0), XMVectorSet(0, 0, z, 0));
	}



	const Metatype* types[] =
	{
			get_metatype<RenderMatrixComponent>(),
			get_metatype<CubeRendererComponent>(),
			get_metatype<StaticTransform>()
	};

	auto* reg = &registry_decs;
	Archetype* blockArchetype = find_or_create_archetype(reg, types, 3);


	for (float z = -20000.0; z < 20000.0; z += 220)
	{
		for (float x = -20000.0; x < 20000.0;x += 220)
		{
			float dist = sqrt(x*x +z*z);
			float yoffset = fmax(0.0f, (dist /3.f) - 1000.f);
			float y = rng::RandomFloat() * 100 - 1000 + yoffset;
			TransformComponent transf;
			transf.position = XMVectorSet(x, y, z, 1.0);
			transf.rotationQuat = XMQuaternionIdentity();
			transf.scale = XMVectorSet(120.0f, 120.0f, 100.0f, 0.0f);
	
			auto e = create_entity_with_archetype(blockArchetype);//registry_decs.CreateEntity(BackgroundBuilding);
	
			RenderMatrixComponent & rend = get_component<RenderMatrixComponent>(reg,e);
			ecs::system::UpdateTransform::build_matrix(transf, rend);
			float rngcolor = rng::RandomFloat() * 0.1 + 0.3;
			get_component<CubeRendererComponent>(reg, e).color = XMFLOAT3(rngcolor, rngcolor, rngcolor);
		}
	}
}

void ECS_GameWorld::update_all(float dt)
{
	rmt_ScopedCPUSample(AllUpdate, 0);
	ApplicationInfo & appInfo = registry_entt.get<ApplicationInfo>();
	//appInfo.TotalEntities = registry_decs.Entities.size() - registry_decs.deletedEntities.size();
	//appInfo.BoidEntities = registry_decs.Entities.size() - registry_decs.deletedEntities.size();
	Bench_End(AllBench);
	appInfo.deltaTime = Bench_GetMiliseconds(AllBench);
	Bench_Start(AllBench);
	BenchmarkInfo bench;
	Bench_Start(bench);


	auto & timec = registry_entt.get<EngineTimeComponent>();
	timec.delta_time = dt;


	ecs::Task root_task = task_engine.placeholder();
	ecs::Task end_task = task_engine.placeholder();
	{
		rmt_ScopedCPUSample(ScheduleUpdate, 0);


		//root_task.name("Root");
		//
		//ecs::Task last_task = root_task;
		//ecs::Task latest_task = last_task;
		//for (auto s : Systems)
		//{
		//	auto temp = last_task;
		//	last_task = s->schedule(registry_entt, task_engine, last_task, latest_task);
		//	latest_task = temp;
		//}
		//
		//end_task = task_engine.placeholder();
		//end_task.gather(last_task);
		//end_task.name("End");

	}



	//std::ofstream myfile;
	//myfile.open("example.txt", std::ios::trunc);
	//myfile << task_engine.dump();
	//myfile.close();
	{
		rmt_ScopedCPUSample(TaskWait, 0);
		//task_engine.wait_for_all();


	}

	{
		rmt_ScopedCPUSample(SimulationUpdate, 0);
		for (auto s : Systems)
		{
			//if (!s->uses_threading)
			//{
				s->update(*this);
			//}
		}
		Bench_End(bench);
		appInfo.SimTime = Bench_GetMiliseconds(bench);
		appInfo.Drawcalls = nDrawcalls;
		appInfo.TotalEntities = registry_decs.live_entities;

		debug_elapsed += dt;
		if (debug_elapsed > 1)
		{
			debug_elapsed = 0;
			debugiterations = 0;
			g_SimpleProfiler->displayunits = g_SimpleProfiler->units;
			g_SimpleProfiler->units.clear();
		}
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
		Renderer->update(*this);
		Bench_End(bench);
		appInfo.RenderTime = Bench_GetMiliseconds(bench);
		appInfo.Drawcalls = Renderer->drawcalls;

	}
	AppInfoUI(appInfo);
	DrawSystemPerformanceUnits(g_SimpleProfiler->displayunits);
	for (auto s : Systems)
	{
		s->cleanup(registry_entt);
	}
}

EngineTimeComponent ECS_GameWorld::GetTime()
{
	return registry_entt.get<EngineTimeComponent>();
}
