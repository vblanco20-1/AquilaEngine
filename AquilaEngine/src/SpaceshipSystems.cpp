#include "SpaceshipSystems.h"
#include "BoidSystems.h"
#include "SimpleProfiler.h"



ecs::TaskEngine::Task SpaceshipMovementSystem::schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::TaskEngine::Task & parent)
{
	const float dt = get_delta_time(registry);
	ecs::TaskEngine::Task task = task_engine.silent_emplace([&, dt]() {
	
		update(registry, dt);
	});
	

	//task_engine.linearize(parent, pre_task, S);

	//task.name("Spaceship Movement System");
	//run after the parent
	task.gather(parent);
	
	return std::move(task);
}

void SpaceshipMovementSystem::update(ECS_Registry &registry, float dt)
{
	rmt_ScopedCPUSample(SpaceshipMovementSystem, 0);
	SCOPE_PROFILE("Spaceship Movement System");
	

	BoidReferenceTag & boidref = registry.get<BoidReferenceTag>();

	//120 fps simulation
	dt = 1.0 / 60.0;
	elapsed = dt;

	auto  posview = registry.view<SpaceshipMovementComponent, TransformComponent>(entt::persistent_t{});

	std::for_each(/*std::execution::par_unseq, */posview.begin(), posview.end(), [&](const auto entity) {


		auto & ship = posview.get<SpaceshipMovementComponent>(entity);
		auto & t = posview.get<TransformComponent>(entity);


		XMVECTOR Mov = ship.Target - t.position;
		Mov = XMVector3Normalize(Mov);
		Mov = Mov * ship.speed * dt;

		ship.Velocity += Mov;


		XMVECTOR OffsetVelocity{ 0.0f,0.0f,0.0f,0.0f };
		boidref.map->Foreach_EntitiesInRadius_Morton(10, t.position, [&](const GridItem2& boid) {

			XMVECTOR Avoidance = t.position - boid.pos;
			float dist = XMVectorGetX(XMVector3Length(Avoidance));
			OffsetVelocity += XMVector3Normalize(Avoidance)*  (1.0f - (std::clamp(dist / 10.0f, 0.0f, 1.0f)));

		});



		ship.Velocity = XMVector3ClampLength(ship.Velocity + OffsetVelocity, 0.0f, ship.speed);

		XMMATRIX rotmat = XMMatrixLookAtLH(t.position, t.position + ship.Velocity * 10, XMVectorSet(0, 1, 0, 0));

		t.rotationQuat = XMQuaternionRotationMatrix(rotmat);

		t.position = t.position + ship.Velocity;
	});
	//}
}

