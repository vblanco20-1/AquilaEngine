#pragma once
#include <PrecompiledHeader.h>
#include "DXShaders.h"
#include "ECSCore.h"
#include "SimpleProfiler.h"



static int nDrawcalls;

namespace ecs::system {


	struct CameraUpdate : public System {
		virtual void update(ECS_GameWorld &world)override;
	};
	struct FrustrumCuller : public System {

		struct QueueTraits : public moodycamel::ConcurrentQueueDefaultTraits
		{
			static const size_t BLOCK_SIZE = 256;		// Use bigger blocks
		};
			
		virtual void update(ECS_GameWorld &world)override;

		void build_view_queues(ECS_GameWorld& world);

		void chull_chunk(DataChunk* chnk, XMVECTOR CamPos, XMVECTOR CamDir);

		void apply_queues(ECS_GameWorld& world);		

	private:
		moodycamel::ConcurrentQueue<decs::EntityID,QueueTraits> SetCulledQueue;
		moodycamel::ConcurrentQueue<decs::EntityID,QueueTraits> RemoveCulledQueue;
	};


	struct CubeRenderer : public System {
		ObjectUniformStruct uniformBuffer;

		void pre_render();
		
		virtual void update(ECS_GameWorld &world)override;
	};

	struct RenderCore : public System {

		RenderCore();
	
		virtual void update(ECS_GameWorld &world)override;
		void render_start();
		void render_end();
		void Present(bool vSync);

		//std::vector<System*> Renderers;
		CubeRenderer* cube_renderer;
		CameraUpdate* cam_updater;
		FrustrumCuller* culler;

		int drawcalls;
		
	};
}

