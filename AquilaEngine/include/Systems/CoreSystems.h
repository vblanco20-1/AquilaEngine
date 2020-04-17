#pragma once
#include "ECSCore.h"
#include "GameWorld.h"

struct RotatorSystem : public System {

	RotatorSystem() {};

	virtual  void update(ECS_GameWorld & world)
	{
		SCOPE_PROFILE("Rotation System-decs");

		auto* reg = &world.registry_decs;		

		float dt = world.GetTime().delta_time;

		reg->for_each([&](RotatorComponent& rotator, TransformComponent& t) {

			

			t.rotationQuat = XMQuaternionMultiply(t.rotationQuat, XMQuaternionRotationAxis(XMLoadFloat3(&rotator.Axis), dt * rotator.rate));
		});
	}
};