#pragma once
#include "ECSCore.h"


struct RotatorSystem : public System {

	RotatorSystem() { uses_threading = true; };


	virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

		const float dt = get_delta_time(registry);
		ecs::Task task = task_engine.silent_emplace([&, dt]() {

			update(registry, dt);
		});

		task.name("Rotator System");
		//run after the parent
		task.gather(parent);
		return std::move(task);
	};


	virtual void update(ECS_Registry &registry, float dt)
	{
		auto  rotview = registry.view<TransformComponent, RotatorComponent>(entt::persistent_t{});

		//rotview.each([&, dt](auto entity, RotationComponent & rotation, RotatorComponent & rotator) {
		//	rotation.Angle += 90.0f * dt;
		//});
		rotview.each([&, dt](auto entity, TransformComponent & t, RotatorComponent & rotator) {

			t.rotationQuat = XMQuaternionMultiply(t.rotationQuat, XMQuaternionRotationAxis(rotator.Axis, dt * rotator.rate));
			//rotation.Angle += 90.0f * dt;
		});
	};
};