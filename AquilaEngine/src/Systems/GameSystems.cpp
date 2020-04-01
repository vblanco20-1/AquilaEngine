
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
	
}

void PlayerCameraSystem::update(ECS_GameWorld& world)
{
	ZoneNamed(PlayerCameraSystem, true);
	XMFLOAT3 CamOffset = XMFLOAT3{ 0.f, 0.f, 0.f };//  (0.f, 0.f, 0.f, 0.f);
	auto &registry = world.registry_entt;
	if (world.registry_entt.has<PlayerInputTag>())
	{
		PlayerInputTag& input = registry.get<PlayerInputTag>();

		CamOffset.z += input.Input.Mousewheel;
		CamOffset.x = (input.Input.MouseX - 500) / 15.f;
		CamOffset.y = -1 * (input.Input.MouseY - 500) / 15.f;

		XMVECTOR Offset = XMVectorSet(0.0f, 0.0f, CamOffset.z, 0.0f);
		//registry.view<PositionComponent, CameraComponent>(entt::persistent_t{}).each([&, dt](auto entity, PositionComponent& campos, CameraComponent& cam) {
		world.registry_decs.for_each([&](EntityID entity, PositionComponent & campos, CameraComponent & cam) {
			XMVECTOR CamForward = XMVector3Normalize(cam.focusPoint - XMLoadFloat3(&campos.Position));
			XMVECTOR CamUp = XMVector3Normalize(cam.upDirection);
			XMVECTOR CamRight = XMVector3Cross(CamForward, XMVectorSet(0, -1, 0, 0));
			XMVECTOR MovOffset = input.Input.MoveForward * CamForward + input.Input.MoveRight * CamRight + g_InputMap.MoveUp * XMVectorSet(0, 1, 0, true);

			campos.Position.x += XMVectorGetX(MovOffset);
			campos.Position.y += XMVectorGetY(MovOffset);
			campos.Position.z += XMVectorGetZ(MovOffset);


			CamForward = XMVector3Rotate(CamForward, XMQuaternionRotationAxis(CamRight, input.Input.MouseDeltaY / 300.0f));
			XMVECTOR CamForwardRotated = XMVector3Rotate(CamForward, XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), input.Input.MoveUp / 3.0f));


			cam.focusPoint = XMLoadFloat3(&campos.Position) + CamForwardRotated;
		});
	}
}

void SpaceshipSpawnSystem::update(ECS_Registry &registry, float dt)
{
	ZoneNamed(SpaceshipSpawnSystem, true);
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
	ZoneNamed(SpaceshipSpawnSystem, true);
	SCOPE_PROFILE("Spawner System ")

	auto* reg = &world.registry_decs;
	float dt = world.GetTime().delta_time;

	{
		ZoneScopedN("Spawner Update", true);

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

	}
	{		
		ZoneScopedNC("Spawner Spawn", tracy::Color::Red, true);
		SpawnUnit unit;
		while (SpawnQueue.try_dequeue(unit)) {

			auto et = reg->new_entity<TransformComponent,
				LifetimeComponent,
				SpaceshipMovementComponent,
				CubeRendererComponent,
				BoidComponent,
				RenderMatrixComponent
			>();

			reg->get_component<TransformComponent>(et).position = DirectX::XMLoadFloat4(&unit.Position);

			reg->get_component<LifetimeComponent>(et).TimeLeft = 5;

			SpaceshipMovementComponent& mv = reg->get_component<SpaceshipMovementComponent>(et);
			mv.Velocity = XMVectorSet(rng::RandomFloat(), rng::RandomFloat(), rng::RandomFloat(), 0) * 2;
			mv.Target = DirectX::XMLoadFloat4(&unit.MoveTarget) + XMVectorSet(rng::RandomFloat(), rng::RandomFloat(), rng::RandomFloat(), 0) * 20;
			mv.speed = 6;

			reg->get_component<CubeRendererComponent>(et).color = unit.Color;
			float offsets = 3;
			{
				auto child = reg->new_entity<TransformComponent,
					TransformParentComponent,
					CubeRendererComponent,
					RenderMatrixComponent
				>();

				reg->get_component<TransformComponent>(child).position = XMVectorSet(0.f, offsets, 0.f, 1.f);
				reg->get_component<TransformComponent>(child).scale = XMVectorSet(0.5f, 0.5f, 10.f, 1.f);

				reg->get_component<TransformParentComponent>(child).Parent = et;			
				reg->get_component<CubeRendererComponent>(child).color = XMFLOAT3(0.f, 0.f, 0.f);
			}
			{
				auto child = reg->new_entity<TransformComponent,
					TransformParentComponent,
					CubeRendererComponent,
					RenderMatrixComponent
				>();

				reg->get_component<TransformComponent>(child).position = XMVectorSet(0.f, -offsets, 0.f, 1.f);
				reg->get_component<TransformComponent>(child).scale = XMVectorSet(0.5f, 0.5f, 10.f, 1.f);

				reg->get_component<TransformParentComponent>(child).Parent = et;
				reg->get_component<CubeRendererComponent>(child).color = XMFLOAT3(0.f, 0.f, 0.f);
			}
			//reg->get_component<TransformComponent>(et).rotationQuat = XMVectorSet(0.f, 0.f, 0.f, 1);
		}
	}
}

void PlayerInputSystem::update(ECS_GameWorld& world)
{
	auto& registry = world.registry_entt;
	ZoneNamed(PlayerInputSystem, true);
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
