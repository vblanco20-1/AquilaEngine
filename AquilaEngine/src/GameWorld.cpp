#include <PrecompiledHeader.h>

#include "GameWorld.h"
#include "RandomUtils.h"
#include "ApplicationInfoUI.h"


#include "EngineGlobals.h"
#include "SimpleProfiler.h"

#include "Timer.h"

#include "Systems/BoidSystems.h"
#include "Systems/SpaceshipSystems.h"
#include "Systems/LifetimeSystems.h"
#include "Systems/GameSystems.h"
#include "Systems/TransformSystems.h"
#include "Systems/RenderSystems.h"

#include "taskflow/taskflow.hpp"
#include "taskflow/core/taskflow.hpp"
#include "taskflow/core/task.hpp"
#include "taskflow/algorithm/for_each.hpp"
#include <bitset>

namespace ecs {
	//tf::Executor executor;
	using TaskEngine = tf::Executor;
	using Task = tf::Task;
	//using SubflowBuilder = typename TaskEngine::SubflowBuilderType;
}

struct TaskStruct {
	ecs::TaskEngine task_engine{ std::thread::hardware_concurrency()-1 };
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

	reg->get_component<TransformComponent>(spawner1).init();
	reg->get_component<TransformComponent>(spawner1).tf->scale = XMVectorSet(1.0f, 10.0f, 10.0f, 0.0f);
	reg->get_component<TransformComponent>(spawner1).tf->position = XMVectorSet(posx, posy, posz, 1.0f);
	reg->get_component<TransformComponent>(spawner1).tf->rotationQuat = XMQuaternionIdentity();


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
			transf.init();
			transf.tf->position = XMVectorSet(x, y, z, 1.0);
			transf.tf->rotationQuat = XMQuaternionIdentity();
			transf.tf->scale = XMVectorSet(120.0f, 120.0f, 100.0f, 0.0f);
	
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


struct PureScheduler {

	std::vector<PureSystemBase*> systemList;

	struct RunUnit {
		std::bitset<32> system_bitflag;
		DataChunk* chunk;
	};

	void run_all(ECS_GameWorld& world, tf::Subflow& subflow) {

		Query q; //fix later
		q.build();
				
		std::vector<RunUnit> units;

		//gather a list of chunks that match an of the queries in the systems
		decs::adv::iterate_matching_archetypes(&world.registry_decs,q,[&](Archetype* arch){
			
			RunUnit unit;
			bool runs = false;
			for (int i = 0; i < systemList.size(); i++) {
				if (decs::adv::archetype_matches(*arch, systemList[i]->query)) {
					runs = true;
					unit.system_bitflag[i] = true;
				}
				else {
					unit.system_bitflag[i] = false;
				}
			}

			if (runs)
			{	
				for (auto c : arch->chunks) {

					units.push_back({unit.system_bitflag,c});
				}
			}
		});

		subflow.for_each( units.begin(), units.end(), [&](RunUnit& unit) {

			for (int i = 0; i < systemList.size(); i++) {
			
				if (unit.system_bitflag[i]) {
					systemList[i]->exec((&world),unit.chunk);
				}
			}
		});
		subflow.join();
	}
};


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

	static bool puresys = true;

	frameNumber = registry_decs.get_singleton<EngineTimeComponent>()->frameNumber;

	tf::Taskflow taskflow;

	Renderer->culler->init_singletons(*this);
	Renderer->cube_renderer->init_singletons(*this);

	{
		ZoneScopedN("Schedule");
		ecs::Task main_simulation = taskflow.emplace([&](tf::Subflow& subflow) {
			ZoneScopedN("Main Simulation");
			//std::atomic<int> boidcount{ 0 };
			PlayerInput_system->update(*this);
			PlayerCamera_system->update(*this);
			if (appInfo.bEnableSimulation) {
				
				Explosion_system->update(*this);

				if (puresys) {

					
					PureScheduler stage;
					stage.systemList.push_back(SpaceshipMovement_system->getAsPureSystem());
					stage.systemList.push_back(Rotator_system->getAsPureSystem());
					stage.systemList.push_back(UpdateTransform_system->update_root_puresys());

					stage.run_all(*this,subflow);
				}
				else {

					SpaceshipMovement_system->update(*this);
					Rotator_system->update(*this);
					UpdateTransform_system->update_root(*this);
				}
				
			}
			
		}).name("main_simulation");

		ecs::Task secondary_simulation = taskflow.emplace([&](tf::Subflow& subflow) {

			PureScheduler stage;
			if (appInfo.bEnableSimulation) {
				stage.systemList.push_back(UpdateTransform_system->update_hierarchy_puresys());
			}
			if (appInfo.bEnableCulling || appInfo.bEnableSimulation) {

				stage.systemList.push_back(Renderer->culler->cull_puresys());
			}
			stage.systemList.push_back(Renderer->cube_renderer->cubebatch_puresys());
			stage.run_all(*this, subflow);

			//auto s1 = subflow.emplace([&](tf::Subflow& subflow) {
			//	if (appInfo.bEnableSimulation) {
			//
			//		UpdateTransform_system->update_hierarchy(*this, subflow);
			//	}
			//});
			//auto s2 = subflow.emplace([&](tf::Subflow& subflow) {
			//	if (appInfo.bEnableCulling || appInfo.bEnableSimulation) {
			//		//Renderer->culler->build_view_queues(*this, subflow);
			//
			//		
			//	
			//});
			//
			//s1.precede(s2);
			//subflow.join();
			
		}).name("secondary_simulation");

		secondary_simulation.succeed(main_simulation);
		

		ecs::Task render_start = taskflow.emplace([&]() {

			Renderer->render_start();
			Renderer->cam_updater->update(*this);
		}).name("render start");
		ecs::Task render_end = taskflow.emplace([&]() {

			Renderer->render_end(*this);
		}).name("render end");
	

		render_end.succeed(render_start);
		
		ecs::Task cullapply_task = taskflow.emplace([&]() {
			Renderer->culler->apply_queues(*this);
		}).name("cull apply");

		ecs::Task cube_task = taskflow.emplace([&](tf::Subflow& subflow) {
			//Renderer->cube_renderer->update_par(*this,subflow);
			}).name("cube process");

		ecs::Task draw_task = taskflow.emplace([&]() {
			Renderer->render_batches(*this);
			}).name("draw meshes");

		ecs::Task boid_fill_task = taskflow.emplace([&](tf::Subflow& subflow) {
			if (appInfo.bEnableSimulation) {
				BoidHash_system->initial_fill(*this,subflow);
			}
		}).name("fill boids");

		ecs::Task boid_sort_task = taskflow.emplace([&](tf::Subflow& subflow) {
			if (appInfo.bEnableSimulation) {
				BoidHash_system->sort_structures(*this,subflow);
			}
		}).name("sort boids");

		ecs::Task lifetime_updates = taskflow.emplace([&]() {
			if (appInfo.bEnableSimulation) {
				SpaceshipSpawn_system->update(*this);
				Destruction_system->update(*this);
			}
		}).name("update lifetimes");

		secondary_simulation.precede(cullapply_task);
		secondary_simulation.precede(cube_task);

		main_simulation.precede(cullapply_task);
		main_simulation.precede(boid_fill_task);

		//render_start.succeed(secondary_simulation);

		//draw_task.precede(cube_task);

		taskflow.linearize({ render_start,draw_task , render_end});
		
		taskflow.linearize({ render_start, cullapply_task, cube_task,render_end });

		taskflow.linearize({ render_start,boid_fill_task,boid_sort_task });

		taskflow.linearize({ boid_sort_task,lifetime_updates });

		//taskflow.linearize({ render_start,boid_fill_task,lifetime_updates });

		//std::ofstream file("graphviz.txt");
		//taskflow.dump(file);
		}
		{
			ZoneNamed(RenderUpdate, true);
			Bench_Start(bench);

			task_engine.run(taskflow).wait();

			

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


EngineTimeComponent ECS_GameWorld::GetTime()
{
	return *registry_decs.get_singleton<EngineTimeComponent>();
}
