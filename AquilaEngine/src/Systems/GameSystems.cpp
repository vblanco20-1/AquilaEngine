
#include "Systems/GameSystems.h"


#include "RandomUtils.h"
#include "ApplicationInfoUI.h"


#include "EngineGlobals.h"
#include "SimpleProfiler.h"
#include "Multivector.h"

#include "Input.h"
#include "Systems/BoidSystems.h"

#include "GameWorld.h"

ecs::Task PlayerCameraSystem::schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent)
{
	//ecs::Task task = task_engine.placeholder();

	const float dt = get_delta_time(registry);
	ecs::Task task = task_engine.silent_emplace([&, dt]() {

		update(registry, dt);
	});

	task.name("Player Camera System ");
	//run after the parent
	task.gather(parent);
	return std::move(task);
}

void PlayerCameraSystem::update(ECS_Registry &registry, float dt)
{
	rmt_ScopedCPUSample(PlayerCameraSystem, 0);
	XMFLOAT3 CamOffset = XMFLOAT3{ 0.f, 0.f, 0.f };//  (0.f, 0.f, 0.f, 0.f);
	if (registry.has<PlayerInputTag>())
	{
		PlayerInputTag & input = registry.get<PlayerInputTag>();

		CamOffset.z += input.Input.Mousewheel;
		CamOffset.x = (input.Input.MouseX - 500) / 15.f;
		CamOffset.y = -1 * (input.Input.MouseY - 500) / 15.f;

		XMVECTOR Offset = XMVectorSet(0.0f, 0.0f, CamOffset.z, 0.0f);
		registry.view<PositionComponent, CameraComponent>(entt::persistent_t{}).each([&, dt](auto entity, PositionComponent & campos, CameraComponent & cam) {

			XMVECTOR CamForward = XMVector3Normalize(cam.focusPoint - XMLoadFloat3(&campos.Position));
			XMVECTOR CamUp = XMVector3Normalize(cam.upDirection);
			XMVECTOR CamRight = XMVector3Cross(CamForward, XMVectorSet(0, -1, 0, 0));
			XMVECTOR MovOffset = input.Input.MoveForward * CamForward + input.Input.MoveRight * CamRight + g_InputMap.MoveUp * XMVectorSet(0, 1, 0, 0);

			campos.Position.x += XMVectorGetX(MovOffset);
			campos.Position.y += XMVectorGetY(MovOffset);
			campos.Position.z += XMVectorGetZ(MovOffset);


			CamForward = XMVector3Rotate(CamForward, XMQuaternionRotationAxis(CamRight, input.Input.MouseDeltaY / 300.0f));
			XMVECTOR CamForwardRotated = XMVector3Rotate(CamForward, XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), input.Input.MoveUp / 3.0f));

			//XMVECTOR FocusOffset = XMVectorSet(input.Input.MouseDeltaX, input.Input.MouseDeltaY,0.0f,0.0f);

			cam.focusPoint = XMLoadFloat3(&campos.Position) + CamForwardRotated;
			//cam.focusPoint.x += ;
			//cam.focusPoint = XMVectorSet(CamOffset.x, CamOffset.y, 0.0f, 1.0f);

		});
	}
}

void SpaceshipSpawnSystem::update(ECS_Registry &registry, float dt)
{
	rmt_ScopedCPUSample(SpaceshipSpawnSystem, 0);
	//return;
	auto  posview = registry.view<SpaceshipSpawnerComponent, TransformComponent>();
	for (auto e : posview)
	{
		auto & spawner = posview.get<SpaceshipSpawnerComponent>(e);

		//..}
		//posview.each([&, dt](auto & e, SpaceshipSpawnerComponent & spawner,const TransformComponent & tr) {

		spawner.Elapsed -= dt;
		while (spawner.Elapsed < 0)
		{
			const auto  tr = posview.get<TransformComponent>(e);
			//XMFLOAT3 Position = pos//TransformComponent//pos.Position;
			spawner.Elapsed += spawner.SpawnRate;

			float roll_x = rng::RandomFloat();  //randomfloat(generator);
			float roll_y = rng::RandomFloat();//randomfloat(generator);
			float roll_z = rng::RandomFloat();//randomfloat(generator);

			auto newe = registry.create();
			registry.assign<CubeRendererComponent>(newe);
			if (XMVectorGetX(tr.position) < 0)
			{
				registry.get<CubeRendererComponent>(newe).color = XMFLOAT3(0.0f, 0.2f, 1.0f);
			}
			else
			{
				registry.get<CubeRendererComponent>(newe).color = XMFLOAT3(1.0f, 0.2f, 0.0f);
			}



			XMVECTOR Position = XMVectorSet(roll_x * spawner.Bounds.x, roll_y * spawner.Bounds.y, roll_z * spawner.Bounds.z, 1.0);
			//Position.x += roll_x * spawner.Bounds.x;
			//Position.y += roll_y * spawner.Bounds.y;
			//Position.z += roll_z * spawner.Bounds.z;

			//registry.assign<PositionComponent>(newe, Position);
			registry.assign<TransformComponent>(newe);
			registry.get<TransformComponent>(newe).position = Position + tr.position;//XMLoadFloat3(&Position);


			registry.assign<RenderMatrixComponent>(newe, XMMATRIX{});
			registry.assign<BoidComponent>(newe);

			//long tip
			auto child_left = registry.create();
			registry.assign<CubeRendererComponent>(child_left);
			registry.assign<TransformComponent>(child_left);
			registry.get<TransformComponent>(child_left).scale = XMVectorSet(0.5f, 0.5f, 5.f, 1.0f);
			registry.get<TransformComponent>(child_left).position = XMVectorSet(1.0f, 0.f, 1.0f, 1.0f);
			registry.get<TransformComponent>(child_left).rotationQuat = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), XMConvertToRadians(-30.0));
			registry.assign<RenderMatrixComponent>(child_left, XMMATRIX{});
			registry.assign<EntityParentComponent>(child_left);

			//registry.get<EntityParentComponent>(child_left).parent = newe;
			//registry.get<EntityParentComponent>(child_left).hierarchyDepth = 1;

			auto child_right = registry.create();
			registry.assign<CubeRendererComponent>(child_right);
			registry.assign<TransformComponent>(child_right);
			registry.get<TransformComponent>(child_right).scale = XMVectorSet(0.5f, 0.5f, 5.f, 1.0f);
			registry.get<TransformComponent>(child_right).position = XMVectorSet(-1.0f, 0.f, 1.0f, 1.0f);
			registry.get<TransformComponent>(child_right).rotationQuat = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), XMConvertToRadians(30.0));//XMVectorSet(-1.0f, 0.f, 1.0f, 1.0f);
			registry.assign<RenderMatrixComponent>(child_right, XMMATRIX{});
			registry.assign<EntityParentComponent>(child_right);

			registry.get<EntityParentComponent>(child_right).SetParent(newe, registry);
			registry.get<EntityParentComponent>(child_left).SetParent(newe, registry);

			const float defaultlifetime = 10.f + rng::RandomFloat() *5.0f;
			registry.assign<LifetimeComponent>(newe, defaultlifetime);
			registry.assign<LifetimeComponent>(child_right, defaultlifetime);
			registry.assign<LifetimeComponent>(child_left, defaultlifetime);

			registry.assign<Level1Transform>(child_right);
			registry.assign<Level1Transform>(child_left);
			//registry.get<EntityParentComponent>(child_right).parent = newe;
			//registry.get<EntityParentComponent>(child_right).hierarchyDepth = 1;


			SpaceshipMovementComponent & mv = registry.assign<SpaceshipMovementComponent>(newe);
			mv.Velocity = XMVectorSet(rng::RandomFloat(), rng::RandomFloat(), rng::RandomFloat(), 0) * 2;
			mv.Target = spawner.ShipMoveTarget + XMVectorSet(rng::RandomFloat(), rng::RandomFloat(), rng::RandomFloat(), 0) * 20;
			mv.speed = 1;




		}
	}//);
}

void SpaceshipSpawnSystem::update(ECS_GameWorld & world)
{
	rmt_ScopedCPUSample(SpaceshipSpawnSystem, 0);
	SCOPE_PROFILE("Spawner System ")

	const Metatype* types[] = {
		get_metatype<SpaceshipSpawnerComponent>(),
		get_metatype<TransformComponent>()
	};

	

	const Metatype* typesship[] =
	{
			get_metatype<TransformComponent>(),
			get_metatype<LifetimeComponent>(),
			get_metatype<SpaceshipMovementComponent>(),
			get_metatype<CubeRendererComponent>(),
			get_metatype<RenderMatrixComponent>(),
			get_metatype<BoidComponent>()
	};

	auto* reg = &world.registry_decs;
	Archetype* spawnerArchetype = find_or_create_archetype(reg,types, 2);
	Archetype* spaceshipArchetype = find_or_create_archetype(reg, typesship, 6);

	float dt = world.GetTime().delta_time;
	reg->for_each([&](SpaceshipSpawnerComponent& spawner, TransformComponent& tr) {
		

		spawner.Elapsed -= dt;
		if (spawner.Elapsed < 0)
		{
			spawner.Elapsed += spawner.SpawnRate;
		
			float roll_x = rng::RandomFloat();
			float roll_y = rng::RandomFloat();
			float roll_z = rng::RandomFloat();
		
			SpawnUnit newSpaceship;		

			float  randomtint = rng::RandomFloat();
			if (XMVectorGetX(tr.position) > 0)
			{
				newSpaceship.Color = XMFLOAT3(1.0f, randomtint, randomtint);		
			}
			else
			{
				newSpaceship.Color = XMFLOAT3(randomtint, randomtint, 1.0f);
			}

			auto pos = tr.position + XMVectorSet(roll_x * spawner.Bounds.x, roll_y * spawner.Bounds.y, roll_z * spawner.Bounds.z, 1.0);
		
			DirectX::XMStoreFloat4(&newSpaceship.Position, pos);
			DirectX::XMStoreFloat4(&newSpaceship.MoveTarget, spawner.ShipMoveTarget);
		
			SpawnQueue.enqueue(newSpaceship);
		}		
	});

	SpawnUnit unit;
	while (SpawnQueue.try_dequeue(unit)) {

		auto et = create_entity_with_archetype(spaceshipArchetype);		
		
		get_component<TransformComponent>(reg,et) = TransformComponent();
		get_component<TransformComponent>(reg,et).position = DirectX::XMLoadFloat4(&unit.Position);
		
		get_component<LifetimeComponent>(reg, et).TimeLeft = 5;
		get_component<BoidComponent>(reg, et) = BoidComponent();
		
		SpaceshipMovementComponent & mv = get_component<SpaceshipMovementComponent>(reg, et);
		mv.Velocity = XMVectorSet(rng::RandomFloat(), rng::RandomFloat(), rng::RandomFloat(), 0) * 2;
		mv.Target =  DirectX::XMLoadFloat4(&unit.MoveTarget)  + XMVectorSet(rng::RandomFloat(), rng::RandomFloat(), rng::RandomFloat(), 0) * 20;
		mv.speed = 6;		
		
		get_component<CubeRendererComponent>(reg,et) = CubeRendererComponent();
		get_component<CubeRendererComponent>(reg,et).color = unit.Color;
		get_component<RenderMatrixComponent>(reg,et) = RenderMatrixComponent();
	}
}

void PlayerInputSystem::update(ECS_Registry &registry, float dt)
{
	rmt_ScopedCPUSample(PlayerInputSystem, 0);
	if (!registry.has<PlayerInputTag>())
	{
		auto player = registry.create();
		registry.assign<PlayerInputTag>(entt::tag_t{}, player);
	}

	g_InputMap.MoveForward = 0;
	g_InputMap.MoveRight = 0;
	g_InputMap.MoveUp = 0;
	//W key
	if (ImGui::IsKeyDown(0x57)) {
		g_InputMap.MoveForward += 1;
	}
	//S key
	if (ImGui::IsKeyDown(0x53)) {
		g_InputMap.MoveForward -= 1;
	}
	//A key
	if (ImGui::IsKeyDown(0x41)) {
		g_InputMap.MoveRight -= 1;
	}
	//D key
	if (ImGui::IsKeyDown(0x44)) {
		g_InputMap.MoveRight += 1;
	}

	//E key
	if (ImGui::IsKeyDown(0x45)) {
		g_InputMap.MoveUp += 1;
	}
	//Q key
	if (ImGui::IsKeyDown(0x51)) {
		g_InputMap.MoveUp -= 1;
	}

	if (bHasFocus && g_InputMap.bShiftDown)
	{
		POINT pos;
		GetCursorPos(&pos);
		pos.x -= 200;
		pos.y -= 200;
		SetCursorPos(200, 200);
		g_InputMap.MouseDeltaX = pos.x;
		g_InputMap.MouseDeltaY = pos.y;
	}


	registry.get<PlayerInputTag>().Input = g_InputMap;

	InputInfo(g_InputMap);
}
