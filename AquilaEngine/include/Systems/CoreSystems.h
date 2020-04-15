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
