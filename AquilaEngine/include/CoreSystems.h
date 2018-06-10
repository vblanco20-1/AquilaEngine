#pragma once
#include "ECSCore.h"


struct RotatorSystem : public System {
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