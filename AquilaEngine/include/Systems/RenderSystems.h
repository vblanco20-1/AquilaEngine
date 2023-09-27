#pragma once
#include <PrecompiledHeader.h>
#include "DXShaders.h"
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

		struct CullMask {
			//popcount
			uint16_t count = 0;
			//128 bytes,  1024 bits 
			//uint64_t mask[16];			
		};
		struct CulledChunk {
			DataChunk* chunk;
			CullMask mask;
		};

		struct ChunkChange {
			DataChunk* chunk;
			bool bVisible;
		};
		struct VisibleRenderChunks {

			std::vector<CulledChunk> visibleChunks;
		};

		struct FrustumCullerStructures {
			moodycamel::ConcurrentQueue<CulledChunk> ChunkQueue;
			moodycamel::ConcurrentQueue<ChunkChange> ChangeChunkQueue;
			moodycamel::ConcurrentQueue<decs::EntityID, QueueTraits> SetCulledQueue;
			moodycamel::ConcurrentQueue<decs::EntityID, QueueTraits> RemoveCulledQueue;
		};

			
		//virtual void update(ECS_GameWorld &world)override;

		void init_singletons(ECS_GameWorld& world);
		decs::PureSystemBase* cull_puresys();

		void build_view_queues(ECS_GameWorld& world, tf::Subflow& sf);
		void apply_queues(ECS_GameWorld& world);		

		static void update_cull_sphere(CullSphere* sphere,TransformComponent* transform,float scale);
		static void update_cull_sphere(CullSphere* sphere, XMVECTOR position, float scale);
	private:
		
	};


	struct CubeRenderer : public System {
		ObjectUniformStruct uniformBuffer;

		void pre_render();
		
		virtual void update(ECS_GameWorld &world)override;

		void build_cube_batches(ECS_GameWorld& world, tf::Subflow& sf);
		void init_singletons(ECS_GameWorld& world);
		decs::PureSystemBase* cubebatch_puresys();


		void render_cube_batch(XMMATRIX* FirstMatrix, XMFLOAT4* FirstColor, uint32_t total_drawcalls);
		virtual void update_par(ECS_GameWorld& world, tf::Subflow& sf)override;
	};

	struct RenderCore : public System {

		RenderCore();
	
		virtual void update(ECS_GameWorld &world)override;
		void render_start();
		void render_end(ECS_GameWorld& world);
		void render_batches(ECS_GameWorld& world);
		void Present(bool vSync);

		CubeRenderer* cube_renderer;
		CameraUpdate* cam_updater;
		FrustrumCuller* culler;

		int drawcalls;		
	};
}

