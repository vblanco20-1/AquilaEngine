
module;
#include <locale>

#include "stdint.h"
#include "concurrentqueue.h"

#include "EngineGlobals.h"
#include "SimpleProfiler.h"

#include "Timer.h"

#include "Systems/BoidSystems.h"
#include "Systems/SpaceshipSystems.h"
#include "Systems/LifetimeSystems.h"
#include "Systems/GameSystems.h"

#include "Systems/RenderSystems.h"

#include "taskflow/taskflow.hpp"

module gameworld;

import transformsystems;
import appinfo;
import "RandomUtils.h";

namespace ecs {
	using TaskEngine = tf::Taskflow;//tf::BasicTaskflow<std::function<void()>>;
	using Task = typename TaskEngine::TaskType;
	using SubflowBuilder = typename TaskEngine::SubflowBuilderType;
}

struct TaskStruct {
	ecs::TaskEngine task_engine{ 1/*std::thread::hardware_concurrency()*/ };
};

void BuildShipSpawner(ECS_GameWorld& world, XMVECTOR  Location, XMVECTOR TargetLocation)
{
	auto* reg = &world.registry_decs;	

	auto spawner1 = reg->new_entity<SpaceshipSpawnerComponent,RenderMatrixComponent
		,TransformComponent, CubeRendererComponent,RotatorComponent,CullSphere,CullBitmask>();

	float posx = XMVectorGetX(Location) + rng::RandomFloat() * 200;
	float posy = XMVectorGetY(Location) + rng::RandomFloat() * 200 + 200;
	float posz = XMVectorGetZ(Location) + rng::RandomFloat() * 200;

	SpaceshipSpawnerComponent  spcomp;
	spcomp.Bounds = XMFLOAT3(10, 50, 50);
	spcomp.Elapsed = 0;
	spcomp.SpawnRate = 0.05;
	spcomp.ShipMoveTarget = TargetLocation;
	
	reg->get_component<SpaceshipSpawnerComponent>(spawner1) = spcomp;
	
	reg->get_component<RotatorComponent>(spawner1).Axis = XMFLOAT3(0.0f, 1.0f, 0.0f);
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


	CullSphere& sphere = reg->get_component<CullSphere>(spawner1);

	ecs::system::FrustrumCuller::update_cull_sphere(&sphere, &reg->get_component<TransformComponent>(spawner1), 1.f);

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
	Explosion_system = new ExplosionFXSystem();

	Renderer = new ecs::system::RenderCore();

	
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
	
			auto e = reg->new_entity<RenderMatrixComponent, CubeRendererComponent, StaticTransform,CullSphere,CullBitmask>();
			CullSphere& sphere = reg->get_component<CullSphere>(e);

			ecs::system::FrustrumCuller::update_cull_sphere(&sphere, &transf, 1.f);
			
			RenderMatrixComponent & rend = reg->get_component<RenderMatrixComponent>(e);
			ecs::system::UpdateTransform::build_matrix(transf, rend);
			float rngcolor = rng::RandomFloat() * 0.1 + 0.3;
			reg->get_component<CubeRendererComponent>( e).color = XMFLOAT3(rngcolor, rngcolor, rngcolor);
		}
	}
}

void ECS_GameWorld::update_all(float dt)
{
	if (!tasksystem) {
		tasksystem = new TaskStruct;
	}
	ecs::TaskEngine& task_engine = tasksystem->task_engine;

	ZoneNamed(AllUpdate, true);
	
	FrameMark;
	
	ApplicationInfo & appInfo = *registry_decs.get_singleton<ApplicationInfo>();

	Bench_End(AllBench);
	appInfo.deltaTime = Bench_GetMiliseconds(AllBench);
	Bench_Start(AllBench);
	BenchmarkInfo bench;
	Bench_Start(bench);


	
	registry_decs.get_singleton<EngineTimeComponent>()->delta_time = dt;
	registry_decs.get_singleton<EngineTimeComponent>()->frameNumber++;

	frameNumber = registry_decs.get_singleton<EngineTimeComponent>()->frameNumber;
	{
		ZoneScopedN("Schedule");
		ecs::Task main_simulation = task_engine.silent_emplace([&]() {
			ZoneScopedN("Main Simulation");
			//std::atomic<int> boidcount{ 0 };
			PlayerInput_system->update(*this);
			PlayerCamera_system->update(*this);
			if (appInfo.bEnableSimulation) {
				
				Explosion_system->update(*this);

				SpaceshipMovement_system->update(*this);
				Rotator_system->update(*this);
				UpdateTransform_system->update_root(*this);
			}
			
		}).name("main_simulation");

		ecs::Task secondary_simulation = task_engine.silent_emplace([&]() {
			if (appInfo.bEnableSimulation) {
				
				UpdateTransform_system->update_hierarchy(*this);
			}
			if (appInfo.bEnableCulling || appInfo.bEnableSimulation) {
				Renderer->culler->build_view_queues(*this);
			}
		}).name("secondary_simulation");

		secondary_simulation.gather(main_simulation);
		
	
		

		ecs::Task render_start = task_engine.silent_emplace([&]() {

			Renderer->render_start();
			Renderer->cam_updater->update(*this);
		}).name("render start");
		ecs::Task render_end = task_engine.silent_emplace([&]() {

			Renderer->render_end();
		}).name("render end");
	

		render_end.gather(render_start);
		
		ecs::Task cullapply_task = task_engine.silent_emplace([&]() {
			Renderer->culler->apply_queues(*this);
		}).name("cull apply");

		ecs::Task cube_task = task_engine.silent_emplace([&]() {
			Renderer->cube_renderer->update(*this);
			}).name("cube process");

		ecs::Task draw_task = task_engine.silent_emplace([&]() {
			Renderer->render_batches(*this);
			}).name("draw meshes");

		ecs::Task boid_fill_task = task_engine.silent_emplace([&]() {
			if (appInfo.bEnableSimulation) {
				BoidHash_system->initial_fill(*this);
			}
		}).name("fill boids");

		ecs::Task boid_sort_task = task_engine.silent_emplace([&]() {
			if (appInfo.bEnableSimulation) {
				BoidHash_system->sort_structures(*this);
			}
		}).name("sort boids");

		ecs::Task lifetime_updates = task_engine.silent_emplace([&]() {
			if (appInfo.bEnableSimulation) {
				SpaceshipSpawn_system->update(*this);
				Destruction_system->update(*this);
			}
		}).name("update lifetimes");

		secondary_simulation.precede(cullapply_task);
		secondary_simulation.precede(cube_task);

		main_simulation.precede(cullapply_task);
		main_simulation.precede(boid_fill_task);

		render_start.precede(main_simulation);

		draw_task.precede(cube_task);

		task_engine.linearize({ render_start, draw_task, render_end });
		
		task_engine.linearize({ render_start, cullapply_task, cube_task});

		task_engine.linearize({ render_start,boid_fill_task,boid_sort_task });

		task_engine.linearize({ render_start,cube_task,lifetime_updates });

		task_engine.linearize({ render_start,boid_fill_task,lifetime_updates });
		}
		{
			ZoneNamed(RenderUpdate, true);
			Bench_Start(bench);

			task_engine.wait_for_all();



			appInfo.TotalEntities = registry_decs.live_entities;
			debug_elapsed += dt;
			if (debug_elapsed > 1)
			{
				debug_elapsed = 0;
				debugiterations = 0;
				g_SimpleProfiler->displayunits = g_SimpleProfiler->units;
				g_SimpleProfiler->units.clear();
			}

			Bench_End(bench);
			appInfo.RenderTime = Bench_GetMiliseconds(bench);
		}
	
	AppInfoUI(appInfo);
	DrawSystemPerformanceUnits(g_SimpleProfiler->displayunits);	
}


//EngineTimeComponent ECS_GameWorld::GetTime()
//{
//	return *registry_decs.get_singleton<EngineTimeComponent>();
//}
