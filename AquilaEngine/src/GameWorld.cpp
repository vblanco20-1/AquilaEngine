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

void BuildShipSpawner(ECS_GameWorld& world, XMVECTOR  Location, XMVECTOR TargetLocation)
{
	auto* reg = &world.registry_decs;	

	auto spawner1 = reg->new_entity<SpaceshipSpawnerComponent,RenderMatrixComponent
		,TransformComponent, CubeRendererComponent,RotatorComponent>();

	float posx = XMVectorGetX(Location) + rng::RandomFloat() * 200;
	float posy = XMVectorGetY(Location) + rng::RandomFloat() * 200 + 200;
	float posz = XMVectorGetZ(Location) + rng::RandomFloat() * 200;

	SpaceshipSpawnerComponent  spcomp;
	spcomp.Bounds = XMFLOAT3(10, 50, 50);
	spcomp.Elapsed = 0;
	spcomp.SpawnRate = 0.05;
	spcomp.ShipMoveTarget = TargetLocation;
	
	reg->get_component<SpaceshipSpawnerComponent>(spawner1) = spcomp;
	
	reg->get_component<RotatorComponent>(spawner1).Axis = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	reg->get_component<RotatorComponent>(spawner1).rate = 1;
	float  randomtint = rng::RandomFloat();
	if (XMVectorGetX(Location) > 0)
	{
		reg->get_component<CubeRendererComponent>(spawner1).color = XMFLOAT3(1.0f, randomtint, randomtint);
	
	}
	else
	{
		reg->get_component<CubeRendererComponent>(spawner1).color = XMFLOAT3(randomtint, randomtint, 1.0f);
	}

	reg->get_component<TransformComponent>(spawner1).scale = XMVectorSet(1.0f, 10.0f, 10.0f, 0.0f);
	reg->get_component<TransformComponent>(spawner1).position = XMVectorSet(posx, posy, posz, 1.0f);
	reg->get_component<TransformComponent>(spawner1).rotationQuat = XMQuaternionIdentity();

	ecs::system::UpdateTransform::build_matrix(reg->get_component<TransformComponent>(spawner1), reg->get_component<RenderMatrixComponent>(spawner1));
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
	auto decs_cam = registry_decs.new_entity<PositionComponent, CameraComponent>();
	registry_decs.get_component<PositionComponent>(decs_cam).Position = XMFLOAT3(0, 600, 2000);
	registry_decs.get_component<CameraComponent>(decs_cam).focusPoint = XMVectorSet(0, 0, 0, 1);
	registry_decs.get_component<CameraComponent>(decs_cam).upDirection = XMVectorSet(0, 1, 0, 0);

	registry_decs.set_singleton<ApplicationInfo>();

	registry_decs.set_singleton<EngineTimeComponent>();

	BoidMap * map = new BoidMap();
	registry_decs.set_singleton<BoidReferenceTag>({ map });
	Bench_Start(AllBench);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	PlayerInput_system					   = new PlayerInputSystem();
	PlayerCamera_system				   = new PlayerCameraSystem();
	SpaceshipSpawn_system			   = new SpaceshipSpawnSystem();
	BoidHash_system						   = new BoidHashSystem();
	SpaceshipMovement_system		   = new SpaceshipMovementSystem();
	Destruction_system					   = new DestructionSystem();
															    
	Rotator_system							   = new RotatorSystem();
	UpdateTransform_system	   = new ecs::system::UpdateTransform();
	

	Renderer = new ecs::system::RenderCore();

	//auto spawner1 = registry.create();
	ApplicationInfo & appInfo = *registry_decs.get_singleton<ApplicationInfo>();
	appInfo.averagedDeltaTime = 0.03f;
	appInfo.deltaTime = 0.012343f;
	appInfo.Drawcalls = 10000;
	appInfo.RenderTime = 1.0f;

	float spawnerstepsystem = 10;
	for (float z = -1000.0; z < 1000.0; z += spawnerstepsystem)
	{
		BuildShipSpawner(*this, XMVectorSet(-500, 0, z, 0), XMVectorSet(0, 0, z, 0));
		BuildShipSpawner(*this, XMVectorSet(500, 0, z, 0), XMVectorSet(0, 0, z, 0));
	}
	
	auto* reg = &registry_decs;

	
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
	
			auto e = reg->new_entity<RenderMatrixComponent, CubeRendererComponent, StaticTransform>();//    create_entity_with_archetype(blockArchetype);//registry_decs.CreateEntity(BackgroundBuilding);
	
			RenderMatrixComponent & rend = reg->get_component<RenderMatrixComponent>(e);
			ecs::system::UpdateTransform::build_matrix(transf, rend);
			float rngcolor = rng::RandomFloat() * 0.1 + 0.3;
			reg->get_component<CubeRendererComponent>( e).color = XMFLOAT3(rngcolor, rngcolor, rngcolor);
		}
	}
}

void ECS_GameWorld::update_all(float dt)
{
	ZoneNamed(AllUpdate, true);
	
	
	ApplicationInfo & appInfo = *registry_decs.get_singleton<ApplicationInfo>();
	//appInfo.TotalEntities = registry_decs.Entities.size() - registry_decs.deletedEntities.size();
	//appInfo.BoidEntities = registry_decs.Entities.size() - registry_decs.deletedEntities.size();
	Bench_End(AllBench);
	appInfo.deltaTime = Bench_GetMiliseconds(AllBench);
	Bench_Start(AllBench);
	BenchmarkInfo bench;
	Bench_Start(bench);

	//auto et = registry_decs.get_singleton<EngineTimeComponent>();
	
	auto& timec = *registry_decs.get_singleton<EngineTimeComponent>();//registry_entt.get<EngineTimeComponent>();
	timec.delta_time = dt;


	ecs::Task root_task = task_engine.placeholder();
	ecs::Task end_task = task_engine.placeholder();
	{
		ZoneNamed(ScheduleUpdate, true);

	}
	   
	{
		ZoneNamed(TaskWait, true);
		//task_engine.wait_for_all();


	}

	{
		ZoneNamed(SimulationUpdate, true);

		PlayerInput_system->update(*this);
		PlayerCamera_system->update(*this);
		if (appInfo.bEnableSimulation) {
			SpaceshipSpawn_system->update(*this);
			Destruction_system->update(*this);
		}

		Query spaceshipQuery;
		spaceshipQuery.with<SpaceshipMovementComponent, TransformComponent>();
		spaceshipQuery.build();
		Query FullTransform;
		FullTransform.with<RenderMatrixComponent, TransformComponent>();
		FullTransform.exclude<StaticTransform>();
		FullTransform.build();

		Query SpaceAndTransform;
		SpaceAndTransform.with<TransformComponent>();
		SpaceAndTransform.exclude<StaticTransform, TransformParentComponent>();
		SpaceAndTransform.build();

		Query ChildTransform;
		ChildTransform.with<RenderMatrixComponent, TransformComponent, TransformParentComponent>();
		ChildTransform.exclude<StaticTransform>();
		ChildTransform.build();
		
		Query StaticObjects;
		StaticObjects.with<CubeRendererComponent,StaticTransform>();
		//ChildTransform.exclude<StaticTransform>();
		StaticObjects.build();

		static std::vector<DataChunk*> spaceship_chunk_cache;
		spaceship_chunk_cache.clear();		

		static std::vector<DataChunk*> child_chunk_cache;
		child_chunk_cache.clear();

		static std::vector<DataChunk*> static_chunk_cache;
		static_chunk_cache.clear();


		{
			ZoneScopedNC("Gather Archetypes", tracy::Color::Green);
			//ZoneScopedNC("Transform Gather Archetypes: ", tracy::Color::Green);

			//world.registry_decs.gather_chunks(FullTransform, full_chunk_cache);
			registry_decs.gather_chunks(ChildTransform, child_chunk_cache);
		
			registry_decs.gather_chunks(StaticObjects, static_chunk_cache);

			registry_decs.gather_chunks(SpaceAndTransform, spaceship_chunk_cache);

		}
		BoidReferenceTag& boidref = *registry_decs.get_singleton<BoidReferenceTag>();


		//for culling
		XMVECTOR CamPos;
		XMVECTOR CamDir;
		registry_decs.for_each([&](EntityID entity, PositionComponent& campos, CameraComponent& cam) {
			CamPos = XMLoadFloat3(&campos.Position);
			CamDir = XMLoadFloat3(&campos.Position) - cam.focusPoint;
			});

	
		std::atomic<int> boidcount{ 0 };
		
		parallel_for_chunk(spaceship_chunk_cache, [&](DataChunk* chnk) {
			auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);
			auto transfarray = get_chunk_array<TransformComponent>(chnk);			
			auto spacearray = get_chunk_array<SpaceshipMovementComponent>(chnk);
			auto rotarray = get_chunk_array<RotatorComponent>(chnk);
			auto cubarray = get_chunk_array<CubeRendererComponent>(chnk);
			auto parentarray = get_chunk_array < TransformParentComponent>(chnk);
			
			if (appInfo.bEnableSimulation) {
			
				//rotating update
				if (rotarray.valid() && transfarray.valid()) {
					update_rotators(dt, chnk);
				}

				//spaceship update----
				if (spacearray.valid() && transfarray.valid()) {
					update_ship_chunk(chnk, boidref, boidcount);
				}
			}
			//transform update
			if (matarray.valid()) {
				update_root_transform(chnk);
			}	
			
			if (appInfo.bEnableCulling) {
				//cull, but only for root objects
				if (cubarray.valid() && !parentarray.valid()) {
					Renderer->culler->chull_chunk(chnk, CamPos, CamDir);
				}
			}			
		});
		if (appInfo.bEnableCulling) {
			parallel_for_chunk(static_chunk_cache, [&](DataChunk* chnk) {
					
					Renderer->culler->chull_chunk(chnk, CamPos, CamDir);				
							
			});
		}
		parallel_for_chunk(child_chunk_cache, [&](DataChunk* chnk) {
			auto matarray = get_chunk_array<RenderMatrixComponent>(chnk);
			auto transfarray = get_chunk_array<TransformComponent>(chnk);
			auto posarray = get_chunk_array<PositionComponent>(chnk);
			auto parentarray = get_chunk_array<TransformParentComponent>(chnk);

			const bool bHasPosition = posarray.chunkOwner == chnk;

			{
				ZoneScopedNC("Transform chunk execute: ", tracy::Color::Red);
				update_root_transform_arrays(chnk, matarray.data, transfarray.data,
					bHasPosition ? posarray.data : nullptr);
			}
			

			{
				ZoneScopedNC("Transform chunk execute: children", tracy::Color::Orange);

				update_children_transform_arrays(&registry_decs, chnk, matarray.data, transfarray.data, parentarray.data);
			}

			//update_root_transform(chnk);
			//update_children_transform(chnk, *this);
			if (appInfo.bEnableCulling) {
				Renderer->culler->chull_chunk(chnk, CamPos, CamDir);
			}
		});

		//SpaceshipMovement_system->update(*this);
		
		

		//Rotator_system->update(*this);
		//UpdateTransform_system->update(*this);

		appInfo.BoidEntities = boidcount.load();
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
		ZoneNamed(RenderUpdate, true);
		Bench_Start(bench);

		ecs::Task render_start = task_engine.silent_emplace([&]() {

			Renderer->render_start();
		});
		ecs::Task render_end = task_engine.silent_emplace([&]() {

			Renderer->render_end();
		});

		render_end.gather(render_start);

		ecs::Task render_task = task_engine.silent_emplace([&]() {

			Renderer->cam_updater->update(*this);			
		});
		ecs::Task cullapply_task = task_engine.silent_emplace([&]() {
			Renderer->culler->apply_queues(*this);
		});

		ecs::Task cube_task = task_engine.silent_emplace([&]() {
			Renderer->cube_renderer->update(*this);
			});

		ecs::Task boid_fill_task = task_engine.silent_emplace([&]() {
			BoidHash_system->initial_fill(*this);
		});

		ecs::Task boid_sort_task = task_engine.silent_emplace([&]() {
		
			BoidHash_system->sort_structures(*this);
		});
		task_engine.linearize({ render_start, render_task, cullapply_task, cube_task, render_end });

		task_engine.linearize({ boid_fill_task,boid_sort_task, render_end });		

		//cullapply_task.gather(boid_fill_task);
		//task.name("Player Camera System ");
		//run after the parent
		

		task_engine.wait_for_all();

		Bench_End(bench);
		appInfo.RenderTime = Bench_GetMiliseconds(bench);
		appInfo.Drawcalls = Renderer->drawcalls;

	}
	AppInfoUI(appInfo);
	DrawSystemPerformanceUnits(g_SimpleProfiler->displayunits);
	for (auto s : Systems)
	{
		//s->cleanup(registry_entt);
	}
}

EngineTimeComponent ECS_GameWorld::GetTime()
{
	return *registry_decs.get_singleton<EngineTimeComponent>();//registry_entt.get<EngineTimeComponent>();
}
