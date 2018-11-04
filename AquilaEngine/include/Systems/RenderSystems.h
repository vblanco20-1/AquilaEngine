#pragma once
#include <PrecompiledHeader.h>
#include "DXShaders.h"
#include "ECSCore.h"
#include "SimpleProfiler.h"



static int nDrawcalls;

namespace ecs::system {


	struct CameraUpdate : public System {


		virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

			ecs::Task task = task_engine.placeholder();
			task.name("Camera System");
			//run after the parent
			task.gather(parent);
			return std::move(task);
		};

		virtual void update(ECS_Registry &registry, float dt);
	};
	struct FrustrumCuller : public System {


		virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

			ecs::Task task = task_engine.placeholder();
			task.name("Culling System");
			//run after the parent
			task.gather(parent);
			return std::move(task);
		};

		virtual void update(ECS_Registry &registry, float dt);
	};


	struct CubeRenderer : public System {
		ObjectUniformStruct uniformBuffer;

		virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent) {

			ecs::Task task = task_engine.placeholder();
			task.name("Cube Renderer System");
			//run after the parent
			task.gather(parent);
			return std::move(task);
		};


		virtual void update(ECS_Registry &registry, float dt);;
	};

	struct RenderCore : public System {

		RenderCore();

		virtual ecs::Task schedule(ECS_Registry &registry, ecs::TaskEngine & task_engine, ecs::Task & parent, ecs::Task & grandparent);;
		virtual void update(ECS_Registry &registry, float dt);

		void render_start();
		void render_end();
		void Present(bool vSync);

		std::vector<System*> Renderers;

	};
}

