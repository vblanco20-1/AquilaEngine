#pragma once
#include "ECSCore.h"
#include "GameWorld.h"

struct RotatorSystem : public System {

	RotatorSystem() { uses_threading = true; };


	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

		const float dt = get_delta_time(registry);
		ecs::Task task = task_engine.silent_emplace([&, dt]() {

		//	update(registry, dt);
		});

		task.name("Rotator System");
		//run after the parent
		task.gather(parent);
		return std::move(task);
	};

	virtual  void update(ECS_GameWorld & world)
	{
		SCOPE_PROFILE("Rotation System-decs");

		auto* reg = &world.registry_decs;		

		float dt = world.GetTime().delta_time;

		reg->for_each([&](RotatorComponent& rotator, TransformComponent& t) {

			t.rotationQuat = XMQuaternionMultiply(t.rotationQuat, XMQuaternionRotationAxis(rotator.Axis, dt * rotator.rate));
		});
	}
};

void update_rotators(float dt,DataChunk* chnk)
{
	ZoneScopedNC("Rotator chunk execute", tracy::Color::Orange);

	auto rotarray = get_chunk_array<RotatorComponent>(chnk);
	auto transfarray = get_chunk_array<TransformComponent>(chnk);	

	for (int i = chnk->header.last - 1; i >= 0; i--)
	{
		RotatorComponent& rotator = rotarray[i];
		TransformComponent& t = transfarray[i];

		t.rotationQuat = XMQuaternionMultiply(t.rotationQuat, XMQuaternionRotationAxis(rotator.Axis, dt * rotator.rate));
	}
}
