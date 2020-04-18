#include <PrecompiledHeader.h>
#include "Systems/GameSystems.h"


#include "RandomUtils.h"
#include "ApplicationInfoUI.h"


#include "EngineGlobals.h"
#include "SimpleProfiler.h"

#include "Input.h"
#include "Systems/BoidSystems.h"

#include "GameWorld.h"

void PlayerCameraSystem::update(ECS_GameWorld& world)
{
	ZoneNamed(PlayerCameraSystem, true);
	XMFLOAT3 CamOffset = XMFLOAT3{ 0.f, 0.f, 0.f };//  (0.f, 0.f, 0.f, 0.f);
	auto &registry = world.registry_decs;
	if (registry.get_singleton<PlayerInputTag>())
	{
		PlayerInputTag& input = *registry.get_singleton<PlayerInputTag>();

		CamOffset.z += input.Input.Mousewheel;
		CamOffset.x = (input.Input.MouseX - 500) / 15.f;
		CamOffset.y = -1 * (input.Input.MouseY - 500) / 15.f;

		XMVECTOR Offset = XMVectorSet(0.0f, 0.0f, CamOffset.z, 0.0f);

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

void SpaceshipSpawnSystem::update(ECS_GameWorld & world)
{
	ZoneNamed(SpaceshipSpawnSystem, true);
	SCOPE_PROFILE("Spawner System ")

	auto* reg = &world.registry_decs;
	float dt = world.GetTime().delta_time;

	{
		ZoneScopedN("Spawner Update");
		ApplicationInfo* appInfo = world.registry_decs.get_singleton<ApplicationInfo>();


		reg->for_each([&](SpaceshipSpawnerComponent& spawner, TransformComponent& tr) {

			spawner.Elapsed -= dt * appInfo->_SpawnRate;
			
			if (spawner.Elapsed < 0)
			{
				spawner.Elapsed += spawner.SpawnRate ;

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

				DirectX::XMStoreFloat3(&newSpaceship.Position, pos);
				DirectX::XMStoreFloat3(&newSpaceship.MoveTarget, spawner.ShipMoveTarget);

				SpawnQueue.enqueue(newSpaceship);
			}
			});

	}
	{		
		ZoneScopedNC("Spawner Spawn", tracy::Color::Red);
		SpawnUnit unit;
		while (SpawnQueue.try_dequeue(unit)) {

			auto et = reg->new_entity<TransformComponent,
				LifetimeComponent,
				SpaceshipMovementComponent,
				CubeRendererComponent,
				BoidComponent,
				RenderMatrixComponent,
				CullSphere
			>();

			reg->get_component<TransformComponent>(et).position = DirectX::XMLoadFloat3(&unit.Position);

			reg->get_component<LifetimeComponent>(et).TimeLeft = 5;

			SpaceshipMovementComponent& mv = reg->get_component<SpaceshipMovementComponent>(et);
			mv.Velocity = XMFLOAT3(rng::RandomFloat() * 2, rng::RandomFloat() * 2, rng::RandomFloat() * 2) ;
					
			
			
			XMVECTOR TVector= XMLoadFloat3(&unit.MoveTarget) + XMVectorSet(rng::RandomFloat() * 20, rng::RandomFloat() * 20, rng::RandomFloat() * 20,0.f);
			
			XMStoreFloat3(&mv.Target, TVector);
			mv.speed = 6;

			reg->get_component<CubeRendererComponent>(et).color = unit.Color;
			float offsets = 3;
			{
				auto child = reg->new_entity<TransformComponent,
					TransformParentComponent,
					CubeRendererComponent,
					RenderMatrixComponent,
					CullSphere
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
					RenderMatrixComponent,
					CullSphere
				>();

				reg->get_component<TransformComponent>(child).position = XMVectorSet(0.f, -offsets, 0.f, 1.f);
				reg->get_component<TransformComponent>(child).scale = XMVectorSet(0.5f, 0.5f, 10.f, 1.f);

				reg->get_component<TransformParentComponent>(child).Parent = et;
				reg->get_component<CubeRendererComponent>(child).color = XMFLOAT3(0.f, 0.f, 0.f);
			}
		}
	}
}

void PlayerInputSystem::update(ECS_GameWorld& world)
{
	auto& registry = world.registry_decs;
	ZoneNamed(PlayerInputSystem, true);
	if (!registry.get_singleton<PlayerInputTag>())
	{
		registry.set_singleton<PlayerInputTag>();
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


	registry.get_singleton<PlayerInputTag>()->Input = g_InputMap;
}

void RotatorSystem::update(ECS_GameWorld& world)
{
	SCOPE_PROFILE("Rotation System-decs");

	auto* reg = &world.registry_decs;

	float dt = world.GetTime().delta_time;

	reg->for_each([&](RotatorComponent& rotator, TransformComponent& t) {



		t.rotationQuat = XMQuaternionMultiply(t.rotationQuat, XMQuaternionRotationAxis(XMLoadFloat3(&rotator.Axis), dt * rotator.rate));
		});
}
