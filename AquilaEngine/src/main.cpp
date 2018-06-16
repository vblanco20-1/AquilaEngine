#include <PrecompiledHeader.h>
#include "../resource.h"

#include <SimpleVertexShader.h>
#include <SimplePixelShader.h>

#include "DXShaders.h"
#include "ApplicationInfoUI.h"
#include "Timer.h"
#include "Input.h"
#include "ECSCore.h"
#include "CoreSystems.h"
#include "BoidSystems.h"
#include "RandomUtils.h"
#include "EngineGlobals.h"
#include "SimpleProfiler.h"
#include "Multivector.h"

// Forward declarations.
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

using namespace DirectX;

bool LoadContent();
void UnloadContent();

void Update(float deltaTime);
void Render();
void Cleanup();


void BuildExplosionEffect(const XMVECTOR Position, ECS_Registry & registry)
{
	auto explosion = registry.create();
	registry.assign<CubeRendererComponent>(explosion);
	registry.get<CubeRendererComponent>(explosion).color = XMFLOAT3(0.0, 1.0, 1.0);
	registry.assign<TransformComponent>(explosion);
	registry.get<TransformComponent>(explosion).position = Position;
	//registry.get<TransformComponent>(explosion) = registry.get<TransformComponent>(e);
	registry.assign<RenderMatrixComponent>(explosion, XMMATRIX{});						
	//
	registry.assign<LifetimeComponent>(explosion, 2.f /5.0f);
	registry.assign < ExplosionFXComponent>(explosion);

}
struct DestructionSystem : public System {

	struct ExplosionSpawnStruct {
		XMFLOAT3 position;
	};
	struct QueueTraits : public moodycamel::ConcurrentQueueDefaultTraits
	{
		static const size_t BLOCK_SIZE = 256;		// Use bigger blocks
	};


	moodycamel::ConcurrentQueue<EntityID, QueueTraits> EntitiesToDelete;
	moodycamel::ConcurrentQueue<ExplosionSpawnStruct, QueueTraits> ExplosionsToSpawn;

	
	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(DestructionSystem, 0);

		
		auto  view = registry.view<LifetimeComponent>(/*entt::persistent_t{}*/);

		{
			rmt_ScopedCPUSample(Destruction_Iterate, 0);

			std::for_each(std::execution::par, view.begin(), view.end(), [&](const auto e) {

				auto &life = view.get(e);

				//view.each([&, dt](auto & e, LifetimeComponent & life) {
				life.TimeLeft -= dt;
				if (life.TimeLeft < 0)
				{
					if (registry.has<SpaceshipMovementComponent>(e))
					{
						//BuildExplosionEffect(registry.get<TransformComponent>(e).position,   registry);
						//was a spaceship, create explosion fx;

						auto & p = registry.get<TransformComponent>(e).position;

						ExplosionSpawnStruct ex;

						ex.position.x = XMVectorGetX(p);
						ex.position.y = XMVectorGetY(p);
						ex.position.z = XMVectorGetZ(p);
						ExplosionsToSpawn.enqueue(ex);


					}
					//registry.destroy(e);
					EntitiesToDelete.enqueue(e);
				}
			});
		}
		{
			rmt_ScopedCPUSample(Destruction_Apply, 0);

			EntityID EntitiesToDestroy[64];
			while (true)
			{
				int dequeued = EntitiesToDelete.try_dequeue_bulk(EntitiesToDestroy, 64);
				if (dequeued > 0)
				{

					for (int i = 0; i < dequeued;i++)
					{
						registry.destroy(EntitiesToDestroy[i]);
					}
				}
				else
				{
					break;
				}
			}
			//while (EntitiesToDelete.try_dequeue_bulk(EntityToDestroy))
			//{
			//	registry.destroy(EntityToDestroy);
			//}
			//ExplosionSpawnStruct ex;
			//while (ExplosionsToSpawn.try_dequeue(ex)) {
			//
			//	BuildExplosionEffect(XMLoadFloat3(&ex.position), registry);
			//}
			ExplosionSpawnStruct ex[64];
			while (true)
			{
				int dequeued = ExplosionsToSpawn.try_dequeue_bulk(ex, 64);
				if (dequeued > 0)
				{

					for (int i = 0; i < dequeued;i++)
					{
						BuildExplosionEffect(XMLoadFloat3(&ex[i].position), registry);
						//registry.destroy(EntitiesToDestroy[i]);
					}
				}
				else
				{
					break;
				}
			}
		}
	}
};

struct ExplosionFXSystem : public System {


	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(ExplosionFXSystem, 0);
		auto  view = registry.view<ExplosionFXComponent,TransformComponent>(entt::persistent_t{});

		std::for_each(std::execution::par_unseq, view.begin(), view.end(), [&](const auto entity) {


			auto[fx, tf] = view.get<ExplosionFXComponent, TransformComponent>(entity);

			
			fx.elapsed += 5.0*dt;
			
			const float sc = 3 + ((1-abs(fx.elapsed-1.0)) / 1.0) * 5;
				tf.scale = XMVectorSet(sc, sc, sc, sc);
			//if (life.TimeLeft < 0)
			//{
			//	if (registry.has<SpaceshipMovementComponent>(e))
			//	{
			//		BuildExplosionEffect(registry.get<TransformComponent>(e).position, registry);
			//		//was a spaceship, create explosion fx;
			//
			//
			//
			//	}
			//
			//	registry.destroy(e);
			//}
		});
	}
};

struct RandomFlusherSystem : public System {


	virtual void update(ECS_Registry &registry, float dt)
	{
		return;
		//auto  posview = registry.view<CubeRendererComponent>(/*entt::persistent_t{}*/);
		//posview.each([&, dt](auto & e,CubeRendererComponent & cube) {			
		//	
		//	cube.randomval += 0.1f / 10.0f;			
		//});
	}
};
struct PlayerCameraSystem : public System {
	virtual void update(ECS_Registry &registry, float dt)
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

				XMVECTOR CamForward= XMVector3Normalize(cam.focusPoint - XMLoadFloat3(&campos.Position));
				XMVECTOR CamUp     = XMVector3Normalize(cam.upDirection);
				XMVECTOR CamRight  = XMVector3Cross(CamForward, XMVectorSet(0, -1, 0, 0));
				XMVECTOR MovOffset = input.Input.MoveForward * CamForward + input.Input.MoveRight * CamRight + g_InputMap.MoveUp * XMVectorSet(0, 1, 0, 0);

				campos.Position.x += XMVectorGetX(MovOffset);
				campos.Position.y += XMVectorGetY(MovOffset);
				campos.Position.z += XMVectorGetZ(MovOffset);
					

				CamForward = XMVector3Rotate(CamForward, XMQuaternionRotationAxis(CamRight, input.Input.MouseDeltaY / 300.0f));
				XMVECTOR CamForwardRotated = XMVector3Rotate(CamForward, XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), input.Input.MouseDeltaX/300.0f )   );

				//XMVECTOR FocusOffset = XMVectorSet(input.Input.MouseDeltaX, input.Input.MouseDeltaY,0.0f,0.0f);

				cam.focusPoint = XMLoadFloat3(&campos.Position) + CamForwardRotated;
				//cam.focusPoint.x += ;
				//cam.focusPoint = XMVectorSet(CamOffset.x, CamOffset.y, 0.0f, 1.0f);

			});
		}
	};
};
struct CameraSystem : public System {
	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(CameraSystem, 0);

		registry.view<PositionComponent, CameraComponent>(entt::persistent_t{}).each([&, dt](auto entity, PositionComponent & campos, CameraComponent & cam) {

			
			XMVECTOR eyePosition = XMVectorSet(campos.Position.x, campos.Position.y, campos.Position.z, 1);; //XMVectorSet(0, 0, -70, 1);
			XMVECTOR focusPoint = cam.focusPoint;// + XMVectorSet(CamOffset.x, CamOffset.y, 0.0f, 0.0f); //XMVectorSet(0, 0, 0, 1);
			XMVECTOR upDirection =XMVectorSet(0, 1, 0, 0);

			Globals->g_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
			Globals->g_d3dDeviceContext->UpdateSubresource(Globals->g_d3dConstantBuffers[CB_Frame], 0, nullptr, &Globals->g_ViewMatrix, 0, 0);

			//rotation.Angle += 90.0f * dt;
		});
	};
};
struct CullingSystem : public System {
	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(CullingSystem, 0);
		SCOPE_PROFILE("Culling System ")
		XMVECTOR CamPos;
		XMVECTOR CamDir;

		registry.view<PositionComponent, CameraComponent>(entt::persistent_t{}).each([&, dt](auto entity, PositionComponent & campos, CameraComponent & cam) {

			//XMFLOAT3::
			CamPos = XMLoadFloat3(&campos.Position);
			//XMFLOAT3 FocalPoint{ XMVectorGetX(cam.focusPoint),XMVectorGetY(cam.focusPoint),XMVectorGetZ(cam.focusPoint) };
			CamDir = XMLoadFloat3(&campos.Position) - cam.focusPoint;
		});

		CamDir = XMVector3Normalize(CamDir);
		auto  posview = registry.view<RenderMatrixComponent, CubeRendererComponent>(entt::persistent_t{});		

		//XMVECTOR VecDir = 
		std::for_each(std::execution::par_unseq, posview.begin(), posview.end(), [&](const auto entity) {


			auto[matrix ,cube] = posview.get<RenderMatrixComponent, CubeRendererComponent>(entity);
		//posview.each([&, dt](auto entity, RenderMatrixComponent & matrix, PositionComponent&posc,CubeRendererComponent &cube) {
			
			XMVECTOR pos = XMVector3Transform(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f) , matrix.Matrix );
			XMVECTOR ToCube = CamPos - pos;
			XMVECTOR angle = XMVector3AngleBetweenVectors(ToCube, CamDir);

			if (XMVectorGetX(angle) < XMConvertToRadians(40))
			{
				cube.bVisible = true;
			}
			else
			{
				cube.bVisible = false;
			}			
		});
	}
};
struct SpaceshipMovementSystem : public System {
	float elapsed{0.0f};
	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(SpaceshipMovementSystem, 0);
		SCOPE_PROFILE("Spaceship Movement System");
		//elapsed -= dt;
		//if (elapsed < 0)
		//{
			//if (!registry.has<BoidReferenceTag>())
			//{
			//	auto player = registry.create();
			//	registry.assign<BoidReferenceTag>(entt::tag_t{}, player,nullptr);
			//}

			BoidReferenceTag & boidref = registry.get<BoidReferenceTag>();
			


			//120 fps simulation
			dt = 1.0 /60.0;
			elapsed = dt;

			auto  posview = registry.view<SpaceshipMovementComponent, TransformComponent>(entt::persistent_t{});

			std::for_each(std::execution::par_unseq, posview.begin(), posview.end(), [&](const auto entity) {
				
				
				auto & ship = posview.get<SpaceshipMovementComponent>(entity);
				auto & t = posview.get<TransformComponent>(entity);
				

				XMVECTOR Mov = ship.Target - t.position;
				Mov = XMVector3Normalize(Mov);
				Mov = Mov * ship.speed * dt;

				ship.Velocity += Mov;

				
				XMVECTOR OffsetVelocity{0.0f,0.0f,0.0f,0.0f};
				boidref.map->Foreach_EntitiesInRadius_Morton(10, t.position, [&](const GridItem2& boid) {

					XMVECTOR Avoidance = t.position - boid.pos;
					float dist = XMVectorGetX(XMVector3Length(Avoidance));
					OffsetVelocity += XMVector3Normalize(Avoidance)*  (1.0f - (std::clamp(dist / 10.0f, 0.0f, 1.0f)));

				});

				

				ship.Velocity = XMVector3ClampLength(ship.Velocity + OffsetVelocity, 0.0f, ship.speed);

				XMMATRIX rotmat = XMMatrixLookAtLH(t.position, t.position + ship.Velocity *10, XMVectorSet(0,1,0,0));

				t.rotationQuat = XMQuaternionRotationMatrix(rotmat);
				
				t.position = t.position + ship.Velocity;
			});
		//}
		
	}
};

struct Level1Transform {};
struct Level2Transform {};
struct SpaceshipSpawnSystem : public System {
	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(SpaceshipSpawnSystem, 0);
		//return;
		auto  posview = registry.view<SpaceshipSpawnerComponent,TransformComponent>();
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
				registry.get<TransformComponent>(child_left).rotationQuat = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0,0), XMConvertToRadians(-30.0));
				registry.assign<RenderMatrixComponent>(child_left, XMMATRIX{});
				registry.assign<EntityParentComponent>(child_left);

				//registry.get<EntityParentComponent>(child_left).parent = newe;
				//registry.get<EntityParentComponent>(child_left).hierarchyDepth = 1;

				auto child_right = registry.create();
				registry.assign<CubeRendererComponent>(child_right);
				registry.assign<TransformComponent>(child_right);
				registry.get<TransformComponent>(child_right).scale = XMVectorSet(0.5f, 0.5f, 5.f, 1.0f);
				registry.get<TransformComponent>(child_right).position = XMVectorSet(-1.0f, 0.f, 1.0f, 1.0f);
				registry.get<TransformComponent>(child_right).rotationQuat = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0,0), XMConvertToRadians(30.0));//XMVectorSet(-1.0f, 0.f, 1.0f, 1.0f);
				registry.assign<RenderMatrixComponent>(child_right, XMMATRIX{});
				registry.assign<EntityParentComponent>(child_right);

				registry.get<EntityParentComponent>(child_right).SetParent( newe, registry);
				registry.get<EntityParentComponent>(child_left).SetParent(  newe,registry);

				const float defaultlifetime = 10.f + rng::RandomFloat() *5.0;
				registry.assign<LifetimeComponent>(newe, defaultlifetime);
				registry.assign<LifetimeComponent>(child_right, defaultlifetime);
				registry.assign<LifetimeComponent>(child_left, defaultlifetime);

				registry.assign<Level1Transform>(child_right);
				registry.assign<Level1Transform>(child_left);
				//registry.get<EntityParentComponent>(child_right).parent = newe;
				//registry.get<EntityParentComponent>(child_right).hierarchyDepth = 1;


				SpaceshipMovementComponent & mv = registry.assign<SpaceshipMovementComponent>(newe);
				mv.Velocity = XMVectorSet(rng::RandomFloat(), rng::RandomFloat(), rng::RandomFloat(), 0) *2;
				mv.Target = spawner.ShipMoveTarget + XMVectorSet(rng::RandomFloat(), rng::RandomFloat(), rng::RandomFloat(), 0) * 20;
				mv.speed = 1;

				

				
			}						
		}//);
	}
};
bool bHasFocus{ true };
using namespace moodycamel;
struct PlayerInputSystem : public System {

	virtual void update(ECS_Registry &registry, float dt)
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
};




struct TransformUpdateSystem : public System {

	
	//struct HierarchyPool {
	//	std::vector<std::pair<RenderMatrixComponent&,EntityID> Entities;
	//};
	//
	struct HierarchyUnit {
		RenderMatrixComponent * mat{ nullptr };
		RenderMatrixComponent * parent{ nullptr };
	};
	
	
	
	struct TransformHierarchy {		
		
		//std::vector< multi_vector<HierarchyUnit,16>> Hierarchy;
		std::vector< ConcurrentQueue<HierarchyUnit>> Queues;
	};

	

	virtual void update(ECS_Registry &registry, float dt)
	{
		rmt_ScopedCPUSample(TransformSystem, 0);
		SCOPE_PROFILE("TransformUpdate System");

		auto  posview = registry.view<TransformComponent, PositionComponent>(entt::persistent_t{});
		auto  scaleview = registry.view<RenderMatrixComponent, TransformComponent>(entt::persistent_t{});
		//auto  rotview = registry.view<RenderMatrixComponent, RotationComponent>(entt::persistent_t{});

		TransformHierarchy Hierarchy;

		{
			rmt_ScopedCPUSample(ApplyPosition, 0);


			std::for_each(std::execution::par_unseq, posview.begin(), posview.end(), [&posview](const auto entity) {

				

				auto[t, p] = posview.get<TransformComponent, PositionComponent>(entity);
				t.position = XMLoadFloat3(&p.Position);
			});
		}
	
		{
			rmt_ScopedCPUSample(CalculateMatrices, 0);

			std::for_each(std::execution::par, scaleview.begin(), scaleview.end(), [&scaleview](const auto entity) {
				
				auto[matrix, t] = scaleview.get<RenderMatrixComponent, TransformComponent>(entity);

				const auto ScaleMat = XMMatrixScalingFromVector(t.scale);
				const auto TranslationMat = XMMatrixTranslationFromVector(t.position);
				const auto RotMat = XMMatrixRotationQuaternion(t.rotationQuat);

				matrix.Matrix = RotMat * (ScaleMat *TranslationMat);
			});
		}
		{
			rmt_ScopedCPUSample(CalculateHierarchy, 0);
			

			int iterations = 0;
			int invalid = 0;
			int lasthierarchy = 0;
			auto hierarchyview = registry.view<EntityParentComponent, RenderMatrixComponent,Level1Transform>(entt::persistent_t{});

			auto lvl1 = registry.view<EntityParentComponent, RenderMatrixComponent, Level1Transform>(entt::persistent_t{});
			auto lvl2 = registry.view<EntityParentComponent, RenderMatrixComponent, Level2Transform>(entt::persistent_t{});


			std::for_each(std::execution::par, lvl1.begin(), lvl1.end(), [&](const auto entity) {
				

				EntityParentComponent & parent = lvl1.get<EntityParentComponent>(entity);
				RenderMatrixComponent & matrix = lvl1.get<RenderMatrixComponent>(entity);

				if (parent.Valid(registry) && parent.hierarchyDepth < 5)
				{
					//if (v.size() < 5) { v.reserve(100) };
					//HierarchyUnit unit;
					//unit.mat = &matrix;
					//unit.parent = &registry.get<RenderMatrixComponent>(parent.parent);
					//Hierarchy.Hierarchy[parent.hierarchyDepth].get_vector().push_back(unit);
					//Hierarchy.Queues[parent.hierarchyDepth].enqueue(unit);
					//iterations++;
					const RenderMatrixComponent & parentmatrix = registry.get<RenderMatrixComponent>(parent.parent);
					matrix.Matrix = matrix.Matrix * parentmatrix.Matrix;
				}
				else
				{
					invalid++;
					matrix.Matrix = XMMatrixScaling(0, 0, 0);
					registry.accommodate<LifetimeComponent>(entity, 0.0f);
				}
			});
			std::for_each(std::execution::par, lvl2.begin(), lvl2.end(), [&](const auto entity) {
				
				EntityParentComponent & parent = lvl2.get<EntityParentComponent>(entity);
				RenderMatrixComponent & matrix = lvl2.get<RenderMatrixComponent>(entity);

				if (parent.Valid(registry) && parent.hierarchyDepth < 5)
				{
					//if (v.size() < 5) { v.reserve(100) };
					//HierarchyUnit unit;
					//unit.mat = &matrix;
					//unit.parent = &registry.get<RenderMatrixComponent>(parent.parent);
					//Hierarchy.Hierarchy[parent.hierarchyDepth].get_vector().push_back(unit);
					//Hierarchy.Queues[parent.hierarchyDepth].enqueue(unit);
					//iterations++;
					const RenderMatrixComponent & parentmatrix = registry.get<RenderMatrixComponent>(parent.parent);
					matrix.Matrix = matrix.Matrix * parentmatrix.Matrix;
				}
				else
				{
					invalid++;
					matrix.Matrix = XMMatrixScaling(0, 0, 0);
					registry.accommodate<LifetimeComponent>(entity, 0.0f);
				}
			});


			//registry.sort<EntityParentComponent>([](const auto &lhs, const auto &rhs) {
			//	return lhs.hierarchyDepth < rhs.hierarchyDepth;
			//});			
			//hierarchyview.each([&, dt](auto entity, EntityParentComponent & parent, RenderMatrixComponent & matrix) {
			//
			//
			//}
			//for (auto& l : Hierarchy.Hierarchy) {
			//	l.clear_all();
			//}
			//
			//Hierarchy.Hierarchy.reserve(5);
			//
			//if (Hierarchy.Queues.size() < 2)
			//{
			//	for (int n = 0; n < 5; n++)
			//	{
			//		//Hierarchy.Hierarchy.push_back(multi_vector<HierarchyUnit, 16>{});
			//		Hierarchy.Queues.push_back(std::move(ConcurrentQueue<HierarchyUnit>{10000}));
			//	}
			//}
			//
			//
			//{
			//	SCOPE_PROFILE("Transform Hiearchjy System - Enqueue");
			//
			//	//Hierarchy.Hierarchy[1].reserve(10000);
			//	//Hierarchy.Hierarchy[2].reserve(10000);
			//	//for (auto& l : Hierarchy.Hierarchy) {
			//	//	l.clear_all();
			//	//}
			//	//auto hierarchyview = registry.view<EntityParentComponent, RenderMatrixComponent>(entt::persistent_t{});
			//	std::for_each(std::execution::par, hierarchyview.begin(), hierarchyview.end(), [&](const auto entity) {
			//		EntityParentComponent & parent = hierarchyview.get<EntityParentComponent>(entity);
			//		RenderMatrixComponent & matrix = hierarchyview.get<RenderMatrixComponent>(entity);
			//		
			//		if (parent.Valid(registry) && parent.hierarchyDepth < 5)
			//		{
			//			//if (v.size() < 5) { v.reserve(100) };
			//			HierarchyUnit unit;
			//			unit.mat = &matrix;
			//			unit.parent = &registry.get<RenderMatrixComponent>(parent.parent);
			//			//Hierarchy.Hierarchy[parent.hierarchyDepth].get_vector().push_back(unit);
			//			Hierarchy.Queues[parent.hierarchyDepth].enqueue(unit);
			//			//iterations++;
			//			//const RenderMatrixComponent & parentmatrix = registry.get<RenderMatrixComponent>(parent.parent);
			//			//matrix.Matrix = matrix.Matrix * parentmatrix.Matrix;
			//		}
			//		else
			//		{
			//			invalid++;
			//			matrix.Matrix = XMMatrixScaling(0, 0, 0);
			//			registry.accommodate<LifetimeComponent>(entity, 0.0f);
			//		}
			//
			//	});
			//}
			{
				//SCOPE_PROFILE("Transform Hiearchjy System - dequeue");
				//
				//for (ConcurrentQueue<HierarchyUnit>&l : Hierarchy.Queues)
				//{
				//	if (l.size_approx() < 1000000)
				//	{
				//		HierarchyUnit unit;
				//
				//		//l.try_dequeue_non_interleaved()
				//		while (l.try_dequeue_non_interleaved(unit))
				//		{
				//			unit.mat->Matrix = unit.mat->Matrix * unit.parent->Matrix;
				//		}
				//	}					
				//}
			}
			
			
														   
			//for (auto & l : Hierarchy.Hierarchy)
			//{
			//	for (auto &v : l.vectors)
			//	{
			//		for (auto & m : v)
			//		{
			//			m.mat->Matrix = m.mat->Matrix * m.parent->Matrix;
			//		}
			//	}
			//}

			//for (auto & l : Hierarchy.Hierarchy)
			//{
			//	std::cout << l.size();
			//}
		}

		//std::cout << iterations << invalid;
		
	};
};

// Clear the color and depth buffers.
void Clear(const FLOAT clearColor[4], FLOAT clearDepth, UINT8 clearStencil)
{
	Globals->g_d3dDeviceContext->ClearRenderTargetView(Globals->g_d3dRenderTargetView, clearColor);
	Globals->g_d3dDeviceContext->ClearDepthStencilView(Globals->g_d3dDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, clearStencil);
}



static int nDrawcalls;

struct CubeRendererSystem: public System {
	ObjectUniformStruct uniformBuffer;

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

		

		int bufferidx =0;
		rmt_ScopedD3D11Sample(IterateCubes);

		//rmt_BeginD3D11Sample(RenderCubeBatch);

		registry.view<RenderMatrixComponent,CubeRendererComponent>().each([&, dt](auto entity, RenderMatrixComponent & matrix, CubeRendererComponent & cube) {
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

class ECS_GameWorld {
public:
	float debug_elapsed;
	int debugiterations{ 0 };
	void Initialize()
	{
		Bench_Start(AllBench);
		ImGui_ImplDX11_NewFrame();
		

		Systems.push_back(new PlayerInputSystem());
		Systems.push_back(new PlayerCameraSystem());
		Systems.push_back(new RandomFlusherSystem());
		Systems.push_back(new SpaceshipSpawnSystem());
		Systems.push_back(new ExplosionFXSystem());

		Systems.push_back(new BoidHashSystem());
		Systems.push_back(new ExplosionFXSystem());
		Systems.push_back(new SpaceshipMovementSystem());
		Systems.push_back(new DestructionSystem());


		Systems.push_back(new RotatorSystem());
		Systems.push_back(new TransformUpdateSystem());
		
		
		Renderer.reset( /*std::make_unique<RenderSystem>(*/new RenderSystem());

		//auto testdestroy = registry.create();
		//std::cout <<"created entity:"<< testdestroy << std::endl;
		//registry.destroy(testdestroy);
		//std::cout << "destroyed entity:" << testdestroy << std::endl;


		//create camera

		auto cam = registry.create();
		registry.assign<PositionComponent>(cam, XMFLOAT3(0, 0, -100));
		registry.assign<CameraComponent>(cam);
		registry.get<CameraComponent>(cam).focusPoint = XMVectorSet(0, 0, 0, 1);

		registry.assign<ApplicationInfo>(entt::tag_t{}, cam);
		//auto spawner1 = registry.create();
		ApplicationInfo & appInfo = registry.get<ApplicationInfo>();
		appInfo.averagedDeltaTime = 0.03f;
		appInfo.deltaTime = 0.012343f;
		appInfo.Drawcalls = 10000;
		appInfo.RenderTime = 1.0f;

		for (float z = -1000;z < 1000; z += 100)
		{
			BuildShipSpawner(registry, XMVectorSet(-500, 0, z, 0), XMVectorSet(0, 0, z, 0));
			BuildShipSpawner(registry, XMVectorSet(500, 0, z, 0), XMVectorSet(0, 0, z, 0));
		}		

		//for (float x = -1000; x < 1000; x+=10)
		//{
		//	for (float y = -100; y < -99; y += 10)
		//	{
		//		for (float z = -1000; z <1000; z += 10)
		//		{
		//			auto e = registry.create();
		//
		//			registry.assign<CubeRendererComponent>(e);
		//
		//			float nx = (x + 1000.0) / 2000.0;
		//			float nz = (z + 1000.0) / 2000.0;
		//			registry.get<CubeRendererComponent>(e).color = XMFLOAT3(nx,0.5,nz) ;
		//
		//			registry.assign<PositionComponent>(e, XMFLOAT3(x, y, z));
		//			registry.assign<RenderMatrixComponent>(e);
		//
		//			registry.assign<TransformComponent>(e);
		//			registry.get<TransformComponent>(e).scale = XMVectorSet(10.0f, 1.0f, 10.0f, 1.0f) ;
		//			//registry.get<TransformComponent>(e).rotationQuat = XMVectorSet(0, 1, 1, 0);
		//			
		//
		//			//XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
		//
		//			registry.assign<RotatorComponent>(e, XMVectorSet(0, 1, 1, 0), 1.0f);
		//			//registry.assign<RotationComponent>(e, rotationAxis, 0.0f);
		//
		//		}
		//	}
		//}
	}

	void Update_All(float dt)
	{
		ApplicationInfo & appInfo = registry.get<ApplicationInfo>();
		appInfo.TotalEntities = registry.size();
		appInfo.BoidEntities = registry.view<BoidComponent>().size();
		Bench_End(AllBench);
		appInfo.deltaTime = Bench_GetMiliseconds(AllBench);
		Bench_Start(AllBench);
		BenchmarkInfo bench;
		Bench_Start(bench);

		{
			rmt_ScopedCPUSample(SimulationUpdate, 0);
			for (auto s : Systems)
			{
				s->update(registry, dt);
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
	BenchmarkInfo AllBench;
	//ApplicationInfo appInfo;
	
	std::unique_ptr<RenderSystem> Renderer;
	ECS_Registry registry;
	std::vector<System*> Systems;
};

ECS_GameWorld *GameWorld{nullptr};

/**
* Initialize the application window.
*/
int InitApplication(HINSTANCE hInstance, int cmdShow)
{
	Remotery* rmt;
	rmt_CreateGlobalInstance(&rmt);

	WNDCLASSEX wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = &WndProc;
	wndClass.hInstance = hInstance;
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = Globals->g_WindowClassName;

	if (!RegisterClassEx(&wndClass))
	{
		return -1;
	}

	RECT windowRect = { 0, 0, Globals->g_WindowWidth, Globals->g_WindowHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	Globals->g_WindowHandle = CreateWindowA(Globals->g_WindowClassName, Globals->g_WindowName,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, nullptr, hInstance, nullptr);

	if (!Globals->g_WindowHandle)
	{
		return -1;
	}

	ShowWindow(Globals->g_WindowHandle, cmdShow);
	UpdateWindow(Globals->g_WindowHandle);

	return 0;
}

// This function was inspired by:
// http://www.rastertek.com/dx11tut03.html
DXGI_RATIONAL QueryRefreshRate(UINT screenWidth, UINT screenHeight, BOOL vsync)
{
	DXGI_RATIONAL refreshRate = { 0, 1 };
	if (vsync)
	{
		IDXGIFactory* factory;
		IDXGIAdapter* adapter;
		IDXGIOutput* adapterOutput;
		DXGI_MODE_DESC* displayModeList;

		// Create a DirectX graphics interface factory.
		HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Could not create DXGIFactory instance."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to create DXGIFactory.");
		}

		hr = factory->EnumAdapters(0, &adapter);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to enumerate adapters."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to enumerate adapters.");
		}

		hr = adapter->EnumOutputs(0, &adapterOutput);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to enumerate adapter outputs."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to enumerate adapter outputs.");
		}

		UINT numDisplayModes;
		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, nullptr);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to query display mode list."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to query display mode list.");
		}

		displayModeList = new DXGI_MODE_DESC[numDisplayModes];
		assert(displayModeList);

		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, displayModeList);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to query display mode list."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to query display mode list.");
		}

		// Now store the refresh rate of the monitor that matches the width and height of the requested screen.
		for (UINT i = 0; i < numDisplayModes; ++i)
		{
			if (displayModeList[i].Width == screenWidth && displayModeList[i].Height == screenHeight)
			{
				refreshRate = displayModeList[i].RefreshRate;
			}
		}

		delete[] displayModeList;
		SafeRelease(adapterOutput);
		SafeRelease(adapter);
		SafeRelease(factory);
	}

	return refreshRate;
}

/**
* Initialize the DirectX device and swap chain.
*/
int InitDirectX(HINSTANCE hInstance, BOOL vSync)
{
	// A window handle must have been created already.
	assert(Globals->g_WindowHandle != 0);

	RECT clientRect;
	GetClientRect(Globals->g_WindowHandle, &clientRect);

	// Compute the exact client dimensions. This will be used
	// to initialize the render targets for our swap chain.
	unsigned int clientWidth = clientRect.right - clientRect.left;
	unsigned int clientHeight = clientRect.bottom - clientRect.top;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = clientWidth;
	swapChainDesc.BufferDesc.Height = clientHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate = QueryRefreshRate(clientWidth, clientHeight, vSync);
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = Globals->g_WindowHandle;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Windowed = TRUE;

	UINT createDeviceFlags = 0;
#if _DEBUG
	createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

	// These are the feature levels that we will accept.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0/*,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1*/
	};

	// This will be the feature level that 
	// is used to create our device and swap chain.
	D3D_FEATURE_LEVEL featureLevel;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, createDeviceFlags, featureLevels, _countof(featureLevels),
		D3D11_SDK_VERSION, &swapChainDesc, &Globals->g_d3dSwapChain, &Globals->g_d3dDevice, &featureLevel,
		&Globals->g_d3dDeviceContext);

	if (hr == E_INVALIDARG)
	{
		hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
			nullptr, createDeviceFlags, &featureLevels[1], _countof(featureLevels) - 1,
			D3D11_SDK_VERSION, &swapChainDesc, &Globals->g_d3dSwapChain, &Globals->g_d3dDevice, &featureLevel,
			&Globals->g_d3dDeviceContext);
	}

	if (FAILED(hr))
	{
		return -1;
	}

	// The Direct3D device and swap chain were successfully created.
	// Now we need to initialize the buffers of the swap chain.
	// Next initialize the back buffer of the swap chain and associate it to a 
	// render target view.
	ID3D11Texture2D* backBuffer;
	hr = Globals->g_d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	if (FAILED(hr))
	{
		return -1;
	}

	hr = Globals->g_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &Globals->g_d3dRenderTargetView);
	if (FAILED(hr))
	{
		return -1;
	}

	SafeRelease(backBuffer);

	// Create the depth buffer for use with the depth/stencil view.
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	ZeroMemory(&depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0; // No CPU access required.
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.Width = clientWidth;
	depthStencilBufferDesc.Height = clientHeight;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.SampleDesc.Quality = 0;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = Globals->g_d3dDevice->CreateTexture2D(&depthStencilBufferDesc, nullptr, &Globals->g_d3dDepthStencilBuffer);
	if (FAILED(hr))
	{
		return -1;
	}

	hr = Globals->g_d3dDevice->CreateDepthStencilView(Globals->g_d3dDepthStencilBuffer, nullptr, &Globals->g_d3dDepthStencilView);
	if (FAILED(hr))
	{
		return -1;
	}

	// Setup depth/stencil state.
	D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc;
	ZeroMemory(&depthStencilStateDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthStencilStateDesc.DepthEnable = TRUE;
	depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilStateDesc.StencilEnable = FALSE;

	hr = Globals->g_d3dDevice->CreateDepthStencilState(&depthStencilStateDesc, &Globals->g_d3dDepthStencilState);

	// Setup rasterizer state.
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state object.
	hr = Globals->g_d3dDevice->CreateRasterizerState(&rasterizerDesc, &Globals->g_d3dRasterizerState);
	if (FAILED(hr))
	{
		return -1;
	}

	// Initialize the viewport to occupy the entire client area.
	Globals->g_Viewport.Width = static_cast<float>(clientWidth);
	Globals->g_Viewport.Height = static_cast<float>(clientHeight);
	Globals->g_Viewport.TopLeftX = 0.0f;
	Globals->g_Viewport.TopLeftY = 0.0f;
	Globals->g_Viewport.MinDepth = 0.0f;
	Globals->g_Viewport.MaxDepth = 1.0f;

	rmt_BindD3D11(Globals->g_d3dDevice, Globals->g_d3dDeviceContext);

	return 0;
}



void UnloadContent()
{
	SafeRelease(Globals->g_d3dConstantBuffers[CB_Appliation]);
	SafeRelease(Globals->g_d3dConstantBuffers[CB_Frame]);
	SafeRelease(Globals->g_d3dConstantBuffers[CB_Object]);
	SafeRelease(Globals->g_d3dIndexBuffer);
	SafeRelease(Globals->g_d3dVertexBuffer);
	SafeRelease(Globals->g_d3dInputLayout);
	SafeRelease(Globals->g_d3dVertexShader);
	SafeRelease(Globals->g_d3dPixelShader);
}

/**
* The main application loop.
*/
int Run()
{
	MSG msg = { 0 };

	static DWORD previousTime = timeGetTime();
	static const float targetFramerate = 30.0f;
	static const float maxTimeStep = 1.0f / targetFramerate;

	GameWorld = new ECS_GameWorld();
	GameWorld->Initialize();

	

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			rmt_ScopedCPUSample(AllFrame, 0);
			rmt_LogText("Time me, too!");

			DWORD currentTime = timeGetTime();
			float deltaTime = (currentTime - previousTime) / 1000.0f;
			previousTime = currentTime;

			// Cap the delta time to the max time step (useful if your 
			// debugging and you don't want the deltaTime value to explode.
			deltaTime = std::min<float>(deltaTime, maxTimeStep);
			
			//Update(deltaTime);
			GameWorld->Update_All(deltaTime);
			
			

			//ImVec2 pos = ImGui::GetCursorPos();
			//pos.x -= 200;
			//pos.y -= 200;
			//ImGui::SetCursorPos({ 200,200 });
			
			//ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
			//ImGui::CaptureMouseFromApp(true);
			//ImGui::SetCursorScreenPos({ 100,100 });
			//SetCursorPos(500, 500);
			//Render();
		}
	}

	return static_cast<int>(msg.wParam);
}


static bool g_bIMGuiInitialized{ false };
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
	UNREFERENCED_PARAMETER(prevInstance);
	UNREFERENCED_PARAMETER(cmdLine);

	Globals = new DXGlobals();
	g_SimpleProfiler = new SimpleProfiler();

	//std::vector<int> coretest;
	//for (int i = 0; i < 1000; i++)
	//{
	//	coretest.push_back(-1);
	//}
	//
	//std::for_each(std::execution::par, coretest.begin(), coretest.end(), [](auto &p) {
	//	p = GetCurrentProcessorNumber();
	//
	//});
	//
	//
	//std::cout << coretest[505 + rng::RandomFloat() * 500];

	// Check for DirectX Math library support.
	if (!XMVerifyCPUSupport())
	{
		MessageBox(nullptr, TEXT("Failed to verify DirectX Math library support."), TEXT("Error"), MB_OK);
		return -1;
	}

	if (InitApplication(hInstance, cmdShow) != 0)
	{
		MessageBox(nullptr, TEXT("Failed to create applicaiton window."), TEXT("Error"), MB_OK);
		return -1;
	}

	if (InitDirectX(hInstance, Globals->g_EnableVSync) != 0)
	{
		MessageBox(nullptr, TEXT("Failed to create DirectX device and swap chain."), TEXT("Error"), MB_OK);
		return -1;
	}

	if (!LoadContent())
	{
		MessageBox(nullptr, TEXT("Failed to load content."), TEXT("Error"), MB_OK);
		return -1;
	}

	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	ImGui_ImplDX11_Init(Globals->g_WindowHandle, Globals->g_d3dDevice, Globals->g_d3dDeviceContext);
	g_bIMGuiInitialized = true;
	// Setup style
	ImGui::StyleColorsDark();

	int returnCode = Run();

	UnloadContent();
	Cleanup();

	return returnCode;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT paintStruct;
	HDC hDC;

	if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
		return true;
	g_InputMap = HandleInputEvent(g_InputMap, hwnd, message, wParam, lParam);

	

	switch (message)
	{
	case WM_SETFOCUS:
		bHasFocus = true;
		break;
	case WM_KILLFOCUS:
		bHasFocus = false;
		break;
	case WM_PAINT:
	{
		hDC = BeginPaint(hwnd, &paintStruct);
		EndPaint(hwnd, &paintStruct);
	}
	break;
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}


void Cleanup()
{
	SafeRelease(Globals->g_d3dDepthStencilView);
	SafeRelease(Globals->g_d3dRenderTargetView);
	SafeRelease(Globals->g_d3dDepthStencilBuffer);
	SafeRelease(Globals->g_d3dDepthStencilState);
	SafeRelease(Globals->g_d3dRasterizerState);
	SafeRelease(Globals->g_d3dSwapChain);
	SafeRelease(Globals->g_d3dDeviceContext);
	SafeRelease(Globals->g_d3dDevice);
}