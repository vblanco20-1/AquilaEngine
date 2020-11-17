//#pragma once

// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Victor Blanco
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
module;
#include <vector>
//import <vector>		;
#include <algorithm>	
#include <typeinfo>	
#include <assert.h>	
#include <unordered_map>
#include <tuple>		
#include <iostream>	
#include <type_traits>
//#pragma warning( disable : 4267 5105 4005)
export module decsm;


//#include <robin_hood.h>
 export {

//forward declarations
  namespace decs {
	using byte = unsigned char;
	static_assert (sizeof(byte) == 1, "size of 1 byte isnt 1 byte");
	constexpr size_t BLOCK_MEMORY_16K = 16384;
	constexpr size_t BLOCK_MEMORY_8K = 8192;

	struct Metatype;
	struct MetatypeHash;
	struct EntityID;
	struct DataChunkHeader;
	struct DataChunk;
	struct ChunkComponentList;
	struct Archetype;
	struct EntityStorage;
	struct Query;
	struct ECSWorld;
}


  namespace decs {
	  export {
	inline constexpr uint64_t hash_64_fnv1a(const char* key, const uint64_t len) {

		uint64_t hash = 0xcbf29ce484222325;
		uint64_t prime = 0x100000001b3;

		for (int i = 0; i < len; ++i) {
			uint8_t value = key[i];
			hash = hash ^ value;
			hash *= prime;
		}

		return hash;
	}

	inline constexpr uint64_t hash_fnv1a(const char* key) {
		uint64_t hash = 0xcbf29ce484222325;
		uint64_t prime = 0x100000001b3;

		int i = 0;
		while (key[i]) {
			uint8_t value = key[i++];
			hash = hash ^ value;
			hash *= prime;
		}

		return hash;
	}

	  template<typename T, int BucketSize>
	 struct HashCache {

		struct Bucket {
			std::array<T, BucketSize> items;
			std::array<size_t, BucketSize> item_hashes;
		};

		HashCache() {
			for (int i = 0; i < 8; ++i) {
				buckets[i] = Bucket{};
			}
		}

		std::array<Bucket, 8> buckets;
	};

	 struct MetatypeHash {
		size_t name_hash{ 0 };
		size_t matcher_hash{ 0 };

		bool operator==(const MetatypeHash& other)const {
			return name_hash == other.name_hash;
		}
		template<typename T>
		static constexpr const char* name_detail() {
			return __FUNCSIG__;
		}

		template<typename T>
		static constexpr size_t hash() {

			static_assert(!std::is_reference_v<T>, "dont send references to hash");
			static_assert(!std::is_const_v<T>, "dont send const to hash");
			return hash_fnv1a(name_detail<T>());
		}
	};
	 struct Metatype {

		using ConstructorFn = void(void*);
		using DestructorFn = void(void*);

		MetatypeHash hash;

		const char* name{ "none" };
		ConstructorFn* constructor;
		DestructorFn* destructor;
		uint16_t size{ 0 };
		uint16_t align{ 0 };

		bool is_empty() const { return align == 0; };


		template<typename T>
		 static constexpr MetatypeHash build_hash() {

			using sanitized = std::remove_const_t<std::remove_reference_t<T>>;

			MetatypeHash hash;

			hash.name_hash = MetatypeHash::hash<sanitized>();
			hash.matcher_hash |= (uint64_t)0x1L << (uint64_t)((hash.name_hash) % 63L);
			return hash;
		};
		template<typename T>
		 static constexpr Metatype build() {
			using sanitized = std::remove_const_t<std::remove_reference_t<T>>;
			Metatype meta{};
			meta.hash = build_hash<T>();
			meta.name = MetatypeHash::name_detail<sanitized>();
			if constexpr (std::is_empty_v<T>)
			{
				meta.align = 0;
				meta.size = 0;
			}
			else {
				meta.align = alignof(T);
				meta.size = sizeof(T);
			}

			meta.constructor = [](void* p) {
				new(p) T{};
			};
			meta.destructor = [](void* p) {
				((T*)p)->~T();
			};

			return meta;
		};
	};

	

	  struct EntityID {
		uint32_t index;
		uint32_t generation;
	};

	  struct alignas(8)DataChunkHeader {
		//pointer to the signature for this block
		struct ChunkComponentList* componentList;
		struct Archetype* ownerArchetype{ nullptr };
		struct DataChunk* prev{ nullptr };
		struct DataChunk* next{ nullptr };
		//max index that has an entity
		int16_t last{ 0 };
		//we want the header aligned to 64 bits
		int16_t pad1{ 0 };
		int16_t pad2{ 0 };
		int16_t pad3{ 0 };

		inline uint64_t* version_ptr(uint16_t index) {
			//take adress of the header
			void* ptr = this + 1;
			//access the memory past it, containing version ids
			uint64_t* ptri = (uint64_t*)ptr;

			return ptri + index;
		}
	};
	  struct alignas(32)DataChunk {
		byte storage[BLOCK_MEMORY_16K - sizeof(DataChunkHeader*)];
		DataChunkHeader *chunk_headers;

		
		inline DataChunkHeader* header(){
			return chunk_headers;
		}
		inline uint32_t count(){
			return header()->last;
		}
		inline uint64_t* version_ptr(uint16_t index) {
			////take adress of the header
			//void* ptr = chunk_headers + 1;
			////access the memory past it, containing version ids
			//uint64_t* ptri = (uint64_t*)ptr;
			//
			//return ptri + index;
			return header()->version_ptr(index);
		}
	};
	static_assert(sizeof(DataChunk) == BLOCK_MEMORY_16K, "chunk size isnt 16kb");

	  struct ChunkComponentList {
		struct CmpPair {
			const Metatype* type;
			MetatypeHash hash;
			uint32_t chunkOffset;
		};
		int16_t chunkCapacity;
		std::vector<CmpPair> components;
	};

	  template<typename T>
	struct ComponentArray {

		ComponentArray() = default;
		ComponentArray(void* pointer, DataChunk* owner, uint16_t cindex) {
			data = (T*)pointer;
			chunkOwner = owner;
			index = cindex;
		}

		const T& operator[](size_t index) const {
			return data[index];
		}
		T& operator[](size_t index) {
			return data[index];
		}
		bool valid()const {
			return data != nullptr;
		}
		T* begin() {
			return data;
		}
		uint64_t version() const {
			return *chunkOwner->version_ptr(index);
		}

		void set_version(uint64_t new_version)  {
			*chunkOwner->version_ptr(index) = new_version;
		}
		T* end() {
			return data + chunkOwner->count();
		}
		int16_t size() {
			return chunkOwner->count();
		}
		T* data{ nullptr };
		DataChunk* chunkOwner{ nullptr };
		uint16_t index;
	};



	  struct Archetype {
		ChunkComponentList* componentList;
		struct ECSWorld* ownerWorld;
		size_t componentHash;
		int full_chunks;
		//full chunks allways on the start of the array
		std::vector<DataChunk*> chunks;
		bool chunklist_need_sort = true;
	};

	  struct EntityStorage {
		DataChunk* chunk;
		uint32_t generation;
		uint16_t chunkIndex;

		bool operator==(const EntityStorage& other) const {
			return chunk == other.chunk && generation == other.generation && chunkIndex == other.chunkIndex;
		}

		bool operator!=(const EntityStorage& other) const {
			return !(other == *this);
		}
	};

	  struct Query {
		std::vector<MetatypeHash> require_comps;
		std::vector<MetatypeHash> exclude_comps;


		size_t require_matcher{ 0 };
		size_t exclude_matcher{ 0 };


		bool built{ false };

		template<typename... C>
		Query& with() {
			require_comps.insert(require_comps.end(), { Metatype::template build_hash<C>()... });

			return *this;
		}

		template<typename... C>
		Query& exclude() {
			exclude_comps.insert(exclude_comps.end(), { Metatype::template build_hash<C>()... });

			return *this;
		}

		Query& build() {
			auto compare_hash = [](const MetatypeHash& A, const MetatypeHash& B) {
				return A.name_hash < B.name_hash;
			};

			auto build_matcher = [](const std::vector<MetatypeHash>& types) {
				size_t and_hash = 0;

				for (auto type : types)
				{
					and_hash |= type.matcher_hash;
				}
				return and_hash;
			};

			auto remove_eid = [](const MetatypeHash& type) {
				return (type == Metatype::template build_hash<EntityID>());
			};
			require_comps.erase(std::remove_if(require_comps.begin(), require_comps.end(), remove_eid), require_comps.end());
			exclude_comps.erase(std::remove_if(exclude_comps.begin(), exclude_comps.end(), remove_eid), exclude_comps.end());

			std::sort(require_comps.begin(), require_comps.end(), compare_hash);
			std::sort(exclude_comps.begin(), exclude_comps.end(), compare_hash);

			require_matcher = build_matcher(require_comps);
			exclude_matcher = build_matcher(exclude_comps);
			built = true;
			return *this;
		}
	};

	  struct ECSWorld {
		std::vector<EntityStorage> entities;
		std::vector<uint32_t> deletedEntities;

		std::unordered_map<uint64_t, std::vector<Archetype*>> archetype_signature_map;
		std::unordered_map<uint64_t, Archetype*> archetype_map;
		std::vector<Archetype*> archetypes;
		//unique archetype hashes
		std::vector<size_t> archetypeHashes;
		//bytemask hash for checking
		std::vector<size_t> archetypeSignatures;

		std::unordered_map<uint64_t, void*> singleton_map;

		int live_entities{ 0 };
		int dead_entities{ 0 };
		inline ECSWorld();

		~ECSWorld()
		{
			for (Archetype* arch : archetypes)
			{
				for (DataChunk* chunk : arch->chunks) {
					delete chunk;
				}
				delete arch;
			}
		};
		//needs push_back(DataChunk*) to work, returns number
		template<typename Container>
		int gather_chunks(Query& query, Container& container);

		template<typename Container, typename Function>
		int gather_chunks_filtered(Query& query, Container& container, Function&& filter);

		template<typename Func>
		void for_each(Query& query, Func&& function, uint64_t execution_id = 0);

		template<typename Func>
		void for_each(Func&& function, uint64_t execution_id = 0);

		template<typename C>
		void add_component(EntityID id, C& comp);
		template<typename C>
		void add_component(EntityID id);

		template<typename C>
		void remove_component(EntityID id);

		template<typename C>
		bool has_component(EntityID id);

		template<typename C>
		C& get_component(EntityID id);

		template<typename C>
		C* set_singleton();
		template<typename C>
		C* set_singleton(C&& singleton);

		template<typename C>
		C* get_singleton();

		template<typename ... Comps>
		inline EntityID new_entity();

		inline void destroy(EntityID eid);

		Archetype* get_empty_archetype() { return archetypes[0]; };
	};

	  template<typename C>
	struct CachedRef
	{
		C* get_from(ECSWorld* world, EntityID target);

		C* pointer;
		EntityStorage storage;
	};

	  inline bool full(DataChunk* chnk) {
		return chnk->header()->last == chnk->header()->componentList->chunkCapacity;
	}

	  template<typename C>
	C* CachedRef<C>::get_from(ECSWorld* world, EntityID target)
	{
		if (world->entities[target.index] != storage) {
			pointer = &world->get_component<C>(target);
			storage = world->entities[target.index];
		}
		return pointer;
	}

	  template<typename T>
	  auto get_chunk_array(DataChunk* chunk) {

		using ActualT = ::std::remove_reference_t<T>;

		if constexpr (std::is_same<ActualT, EntityID>::value)
		{
			EntityID* ptr = ((EntityID*)chunk);
			return ComponentArray<EntityID>(ptr, chunk,-1);
		}
		else {
			constexpr MetatypeHash hash = Metatype::template build_hash<ActualT>();
			const unsigned int ncomps = chunk->header()->componentList->components.size();
			for(uint16_t i = 0 ; i < ncomps ;i++){
				auto cmp = chunk->header()->componentList->components[i];
			//for (auto cmp : chunk->header.componentList->components) {
				if (cmp.hash == hash)
				{
					void* ptr = (void*)((byte*)chunk + cmp.chunkOffset);

					return ComponentArray<ActualT>(ptr, chunk,i);
				}
			}

			return ComponentArray<ActualT>();
		}
	}

	namespace adv {
		

		//forward declarations
		inline int insert_entity_in_chunk(DataChunk* chunk, EntityID EID, bool bInitializeConstructors = true);
		inline EntityID erase_entity_in_chunk(DataChunk* chunk, uint16_t index);
		inline DataChunk* build_chunk(ChunkComponentList* cmpList);


		template<typename... Type>
		struct type_list {};

		template<typename Class, typename Ret, typename... Args>
		type_list<Args...> args(Ret(Class::*)(Args...) const);

		template<typename T>
		static const Metatype* get_metatype() {
			static const Metatype* mt = []() {
				constexpr size_t name_hash = Metatype::template build_hash<T>().name_hash;
				//has stable pointers, use name_hash for it
				static std::unordered_map<uint64_t, decs::Metatype> metatype_cache;

				auto type = metatype_cache.find(name_hash);
				if (type == metatype_cache.end()) {
					constexpr Metatype newtype = Metatype::build<T>();
					metatype_cache[name_hash] = newtype;
				}
				return &metatype_cache[name_hash];
			}();
			return mt;
		}
		
		inline void reorder_chunks(Archetype* arch) {
			
			int archsize = arch->componentList->chunkCapacity;
			std::partition(arch->chunks.begin(), arch->chunks.end(), [archsize](DataChunk* cnk) {
				return cnk->count() == archsize;
				});
			arch->chunklist_need_sort = false;
		}
		//reorder archetype with the fullness
		inline void set_chunk_full(DataChunk* chunk) {

			Archetype* arch = chunk->header()->ownerArchetype;
			if (arch) {
				arch->full_chunks++;

				arch->chunklist_need_sort = true;
				//do it properly later
				//reorder_chunks(arch);
			}


		}
		

		//reorder archetype with the fullness
		inline void set_chunk_partial(DataChunk* chunk) {
			Archetype* arch = chunk->header()->ownerArchetype;
			arch->full_chunks--;

			//do it properly later
			arch->chunklist_need_sort = true;



			//reorder_chunks(arch);
		}

		inline ChunkComponentList* build_component_list(const Metatype** types, size_t count) {
			ChunkComponentList* list = new ChunkComponentList();

			int compsize = sizeof(EntityID);
			for (size_t i = 0; i < count; i++) {

				compsize += types[i]->size;
			}
			size_t versionsIdsSpace = (sizeof(uint64_t) * count);

			size_t availibleStorage = sizeof(DataChunk::storage) - versionsIdsSpace;
			//2 less than the real count to account for sizes and give some slack
			size_t itemCount = (availibleStorage / compsize) - 2;

			uint32_t offsets = 0;
			//reserve entity ids at the start
			offsets += sizeof(EntityID) * itemCount;

			for (size_t i = 0; i < count; i++) {
				const Metatype* type = types[i];

				const bool is_zero_sized = type->align == 0;
				if (!is_zero_sized) {
					//align properly
					size_t remainder = offsets % type->align;
					size_t oset = type->align - remainder;
					offsets += oset;
				}

				list->components.push_back({ type,type->hash,offsets });

				if (!is_zero_sized) {
					offsets += type->size * (itemCount);
				}

			}

			//implement proper size handling later
			assert(offsets <= BLOCK_MEMORY_16K);

			list->chunkCapacity = itemCount;

			return list;
		}

		inline size_t build_signature(const Metatype** types, size_t count) {
			size_t and_hash = 0;
			//for (auto m : types)
			for (int i = 0; i < count; i++)
			{
				//consider if the fancy hash is needed, there is a big slowdown
				//size_t keyhash = ash_64_fnv1a(&types[i]->name_hash, sizeof(size_t));
				//size_t keyhash = types[i]->name_hash;

				and_hash |= types[i]->hash.matcher_hash;
				//and_hash |=(uint64_t)0x1L << (uint64_t)((types[i]->name_hash) % 63L);
			}
			return and_hash;
		}

		inline DataChunk* create_chunk_for_archetype(Archetype* arch) {
			DataChunk* chunk = build_chunk(arch->componentList);

			chunk->header()->ownerArchetype = arch;
			arch->chunks.push_back(chunk);
			return chunk;
		}
		inline void delete_chunk_header(DataChunkHeader* header) {
			header->~DataChunkHeader();
			free(header);
		}
		inline void delete_chunk_from_archetype(DataChunk* chunk) {
			Archetype* owner = chunk->header()->ownerArchetype;
			DataChunk* backChunk = owner->chunks.back();

			if (backChunk != chunk) {
				for (int i = 0; i < owner->chunks.size(); i++) {
					if (owner->chunks[i] == chunk) {
						owner->chunks[i] = backChunk;
					}
				}
			}
			owner->chunks.pop_back();
			chunk->chunk_headers->~DataChunkHeader();
			free(chunk->chunk_headers);
			delete chunk;

		}
		inline bool compare_metatypes(const Metatype* A, const Metatype* B) {
			//return A->name_hash < B->name_hash;
			return A < B;
		}
		inline size_t join_metatypes(const Metatype* ATypes[], size_t Acount, const Metatype* BTypes[], size_t Bcount, const Metatype* output[]) {

			const Metatype** AEnd = ATypes + Acount;
			const Metatype** BEnd = BTypes + Bcount;

			const Metatype** A = ATypes;
			const Metatype** B = BTypes;
			const Metatype** C = output;

			while (true)
			{
				if (A == AEnd) {
					std::copy(B, BEnd, C);
				}
				if (B == BEnd) {
					std::copy(A, AEnd, C);
				}

				if (*A < *B) { *C = *A; ++A; }
				else if (*B < *A) { *C = *B; ++B; }
				else { *C = *A; ++A; ++B; }
				++C;
			}

			return C - output;
		}

		inline void sort_metatypes(const Metatype** types, size_t count) {
			std::sort(types, types + count, [](const Metatype* A, const Metatype* B) {
				return compare_metatypes(A, B);
				});
		}
		inline bool is_sorted(const Metatype** types, size_t count) {
			for (int i = 0; i < count - 1; i++) {
				if (types[i] > types[i + 1]) {
					return false;
				}
			}
			return true;
		}

		inline Archetype* find_or_create_archetype(ECSWorld* world, const Metatype** types, size_t count) {
			const Metatype* temporalMetatypeArray[32];
			assert(count < 32);

			const Metatype** typelist;

			if (false) {//!is_sorted(types, count)) {
				for (int i = 0; i < count; i++) {
					temporalMetatypeArray[i] = types[i];

				}
				sort_metatypes(temporalMetatypeArray, count);
				typelist = temporalMetatypeArray;
			}
			else {
				typelist = types;
			}

			const uint64_t matcher = build_signature(typelist, count);

			//try in the hashmap
			auto iter = world->archetype_signature_map.find(matcher);
			if (iter != world->archetype_signature_map.end()) {

				auto& archvec = iter->second;//world->archetype_signature_map[matcher];
				for (int i = 0; i < archvec.size(); i++) {

					auto componentList = archvec[i]->componentList;
					int ccount = componentList->components.size();
					if (ccount == count) {
						for (int j = 0; j < ccount; j++) {

							if (componentList->components[j].type != typelist[j])
							{
								//mismatch, inmediately continue
								goto contA;
							}
						}

						//everything matched. Found. Return inmediately
						return archvec[i];
					}

				contA:;
				}
			}

			//not found, create a new one

			Archetype* newArch = new Archetype();

			newArch->full_chunks = 0;
			newArch->componentList = build_component_list(typelist, count);
			newArch->componentHash = matcher;
			newArch->ownerWorld = world;
			world->archetypes.push_back(newArch);
			world->archetypeSignatures.push_back(matcher);
			world->archetype_signature_map[matcher].push_back(newArch);

			//we want archs to allways have 1 chunk at least, create initial
			create_chunk_for_archetype(newArch);
			return newArch;
		}


		inline EntityID allocate_entity(ECSWorld* world) {
			EntityID newID;
			if (world->dead_entities == 0) {
				int index = world->entities.size();

				EntityStorage newStorage;
				newStorage.chunk = nullptr;
				newStorage.chunkIndex = 0;
				newStorage.generation = 1;

				world->entities.push_back(newStorage);

				newID.generation = 1;
				newID.index = index;
			}
			else {
				int index = world->deletedEntities.back();
				world->deletedEntities.pop_back();

				world->entities[index].generation++;

				newID.generation = world->entities[index].generation;
				newID.index = index;
				world->dead_entities--;
			}

			world->live_entities++;
			return newID;
		}

		inline bool is_entity_valid(ECSWorld* world, EntityID id) {
			//index check
			if (world->entities.size() > id.index && id.index >= 0) {

				//generation check
				if (id.generation != 0 && world->entities[id.index].generation == id.generation)
				{
					return true;
				}
			}
			return false;
		}
		inline void deallocate_entity(ECSWorld* world, EntityID id) {

			//todo add valid check
			world->deletedEntities.push_back(id.index);
			world->entities[id.index].generation++;
			world->entities[id.index].chunk = nullptr;
			world->entities[id.index].chunkIndex = 0;

			world->live_entities--;
			world->dead_entities++;
		}

		inline void destroy_entity(ECSWorld* world, EntityID id) {
			assert(is_entity_valid(world, id));
			erase_entity_in_chunk(world->entities[id.index].chunk, world->entities[id.index].chunkIndex);
			deallocate_entity(world, id);
		}
		inline DataChunk* find_free_chunk(Archetype* arch) {
			DataChunk* targetChunk = nullptr;
			if (arch->chunks.size() == 0) {
				targetChunk = create_chunk_for_archetype(arch);
			}
			else {
				if (arch->chunklist_need_sort) {
					reorder_chunks(arch);
				}
				targetChunk = arch->chunks[arch->chunks.size() - 1];
				//chunk is full, create a new one
				if (targetChunk->count() == arch->componentList->chunkCapacity) {
					targetChunk = create_chunk_for_archetype(arch);
				}
			}
			return targetChunk;
		}


		inline void move_entity_to_archetype(Archetype* newarch, EntityID id, bool bInitializeConstructors = true) {

			//insert into new chunk
			DataChunk* oldChunk = newarch->ownerWorld->entities[id.index].chunk;
			DataChunk* newChunk = find_free_chunk(newarch);

			int newindex = insert_entity_in_chunk(newChunk, id, bInitializeConstructors);
			int oldindex = newarch->ownerWorld->entities[id.index].chunkIndex;

			int oldNcomps = oldChunk->header()->componentList->components.size();
			int newNcomps = newChunk->header()->componentList->components.size();

			auto& oldClist = oldChunk->header()->componentList;
			auto& newClist = newChunk->header()->componentList;

			//copy all data from old chunk into new chunk
			//bad iteration, fix later

			struct Merge {
				int msize;
				int idxOld;
				int idxNew;
			};
			int mergcount = 0;
			Merge mergarray[32];

			for (int i = 0; i < oldNcomps; i++) {
				const Metatype* mtCp1 = oldClist->components[i].type;
				if (!mtCp1->is_empty()) {
					for (int j = 0; j < newNcomps; j++) {
						const Metatype* mtCp2 = newClist->components[j].type;

						//pointers are stable
						if (mtCp2 == mtCp1) {
							mergarray[mergcount].idxNew = j;
							mergarray[mergcount].idxOld = i;
							mergarray[mergcount].msize = mtCp1->size;
							mergcount++;
						}
					}
				}
			}

			for (int i = 0; i < mergcount; i++) {
				//const Metatype* mtCp1 = mergarray[i].mtype;

				//pointer for old location in old chunk
				void* ptrOld = (void*)((byte*)oldChunk + oldClist->components[mergarray[i].idxOld].chunkOffset + (mergarray[i].msize * oldindex));

				//pointer for new location in new chunk
				void* ptrNew = (void*)((byte*)newChunk + newClist->components[mergarray[i].idxNew].chunkOffset + (mergarray[i].msize * newindex));

				//memcopy component data from old to new
				memcpy(ptrNew, ptrOld, mergarray[i].msize);
			}

			//delete entity from old chunk
			erase_entity_in_chunk(oldChunk, oldindex);

			//assign entity chunk data
			newarch->ownerWorld->entities[id.index].chunk = newChunk;
			newarch->ownerWorld->entities[id.index].chunkIndex = newindex;
		}
		inline void set_entity_archetype(Archetype* arch, EntityID id) {

			//if chunk is null, we are a empty entity
			if (arch->ownerWorld->entities[id.index].chunk == nullptr) {

				DataChunk* targetChunk = find_free_chunk(arch);

				int index = insert_entity_in_chunk(targetChunk, id);
				arch->ownerWorld->entities[id.index].chunk = targetChunk;
				arch->ownerWorld->entities[id.index].chunkIndex = index;
			}
			else {
				move_entity_to_archetype(arch, id, false);
			}
		}
		inline EntityID create_entity_with_archetype(Archetype* arch) {
			ECSWorld* world = arch->ownerWorld;

			EntityID newID = allocate_entity(world);

			set_entity_archetype(arch, newID);

			return newID;
		}
		inline Archetype* get_entity_archetype(ECSWorld* world, EntityID id)
		{
			assert(is_entity_valid(world, id));

			return world->entities[id.index].chunk->header()->ownerArchetype;
		}


		template<typename C>
		bool has_component(ECSWorld* world, EntityID id);

		template<typename C>
		C& get_entity_component(ECSWorld* world, EntityID id);

		template<typename C>
		void add_component_to_entity(ECSWorld* world, EntityID id, C&& comp);

		template<typename C>
		void remove_component_from_entity(ECSWorld* world, EntityID id);

		template<typename F>
		void iterate_matching_archetypes(ECSWorld* world, const Query& query, F&& function) {

			for (int i = 0; i < world->archetypeSignatures.size(); i++)
			{
				//if there is a good match, doing an and not be 0
				uint64_t includeTest = world->archetypeSignatures[i] & query.require_matcher;

				//implement later
				uint64_t excludeTest = world->archetypeSignatures[i] & query.exclude_matcher;
				if (includeTest != 0) {

					auto componentList = world->archetypes[i]->componentList;

					//might match an excluded component, check here
					if (excludeTest != 0) {

						bool invalid = false;
						//dumb algo, optimize later					
						for (int mtA = 0; mtA < query.exclude_comps.size(); mtA++) {

							for (auto cmp : componentList->components) {

								if (cmp.type->hash == query.exclude_comps[mtA]) {
									//any check and we out
									invalid = true;
									break;
								}
							}
							if (invalid) {
								break;
							}
						}
						if (invalid) {
							continue;
						}
					}

					//dumb algo, optimize later
					int matches = 0;
					for (int mtA = 0; mtA < query.require_comps.size(); mtA++) {

						for (auto cmp : componentList->components) {

							if (cmp.type->hash == query.require_comps[mtA]) {
								matches++;
								break;
							}
						}
					}
					//all perfect
					if (matches == query.require_comps.size()) {

						function(world->archetypes[i]);
					}
				}


			}

		}



		template<typename C>
		C& get_entity_component(ECSWorld* world, EntityID id)
		{

			EntityStorage& storage = world->entities[id.index];

			auto acrray = get_chunk_array<C>(storage.chunk);
			assert(acrray.chunkOwner != nullptr);
			return acrray[storage.chunkIndex];
		}


		template<typename C>
		bool has_component(ECSWorld* world, EntityID id)
		{
			EntityStorage& storage = world->entities[id.index];

			auto acrray = get_chunk_array<C>(storage.chunk);
			return acrray.chunkOwner != nullptr;
		}
		//create a new archetype by adding a component to another archetype
		inline Archetype* make_archetype_by_adding_component(Archetype* oldarch, const Metatype* addtype) {
			const Metatype* temporalMetatypeArray[32];

			ChunkComponentList* oldlist = oldarch->componentList;
			bool typeFound = false;
			int lenght = oldlist->components.size();
			for (int i = 0; i < oldlist->components.size(); i++) {
				temporalMetatypeArray[i] = oldlist->components[i].type;

				//the pointers for metatypes are allways fully stable
				if (temporalMetatypeArray[i] == addtype) {
					typeFound = true;
				}
			}

			Archetype* newArch = oldarch;
			if (!typeFound) {

				temporalMetatypeArray[lenght] = addtype;
				sort_metatypes(temporalMetatypeArray, lenght + 1);
				lenght++;

				newArch = find_or_create_archetype(oldarch->ownerWorld, temporalMetatypeArray, lenght);
				return newArch;
			}
			else {
				return oldarch;
			}
		}

		//create a new archetype by rempving a component to another archetype
		inline Archetype* make_archetype_by_removing_component(Archetype* oldarch, const Metatype* type) {
			const Metatype* temporalMetatypeArray[32];

			ChunkComponentList* oldlist = oldarch->componentList;
			bool typeFound = false;
			int lenght = oldlist->components.size();
			for (int i = 0; i < lenght; i++) {
				temporalMetatypeArray[i] = oldlist->components[i].type;

				//the pointers for metatypes are allways fully stable
				if (temporalMetatypeArray[i] == type) {
					typeFound = true;
					//swap last
					temporalMetatypeArray[i] = oldlist->components[lenght - 1].type;
				}
			}

			Archetype* newArch = oldarch;
			if (typeFound) {
				lenght--;
				sort_metatypes(temporalMetatypeArray, lenght);

				newArch = find_or_create_archetype(oldarch->ownerWorld, temporalMetatypeArray, lenght);
				return newArch;
			}
			else {
				return oldarch;
			}
		}

		template<typename C>
		void add_component_to_entity(ECSWorld* world, EntityID id)
		{
			//const Metatype* temporalMetatypeArray[32];

			const Metatype* type = get_metatype<C>();

			Archetype* oldarch = get_entity_archetype(world, id);
			Archetype* newarch = make_archetype_by_adding_component(oldarch, type);
			set_entity_archetype(newarch, id);
#if 0
			ChunkComponentList* oldlist = oldarch->componentList;
			bool typeFound = false;
			int lenght = oldlist->components.size();
			for (int i = 0; i < oldlist->components.size(); i++) {
				temporalMetatypeArray[i] = oldlist->components[i].type;

				//the pointers for metatypes are allways fully stable
				if (temporalMetatypeArray[i] == type) {
					typeFound = true;
				}
			}

			Archetype* newArch = oldarch;
			if (!typeFound) {

				temporalMetatypeArray[lenght] = type;
				sort_metatypes(temporalMetatypeArray, lenght + 1);
				lenght++;

				newArch = find_or_create_archetype(world, temporalMetatypeArray, lenght);

				set_entity_archetype(newArch, id);
			}
#endif
		}
		template<typename C>
		void add_component_to_entity(ECSWorld* world, EntityID id, C& comp)
		{
			const Metatype* type = get_metatype<C>();

			add_component_to_entity<C>(world, id);


			//optimize later
			if (!type->is_empty()) {
				get_entity_component<C>(world, id) = comp;
			}
		}

		inline DataChunkHeader* allocate_chunk_header(ChunkComponentList* cmpList) {
			size_t allocsize = sizeof(DataChunkHeader);
			allocsize += cmpList->components.size() * sizeof(uint64_t) + 4;

			void* headeralloc = malloc(allocsize);

			DataChunkHeader* newheader = new(headeralloc) DataChunkHeader{};
			return newheader;
		}

		inline void change_chunk_archetype(DataChunk* chnk, Archetype* newArch) {
			
			//remove chunk from last archetype
			Archetype* old_archetype = chnk->header()->ownerArchetype;

			if (old_archetype == newArch) return;


			DataChunk* backChunk = old_archetype->chunks.back();

			if (backChunk != chnk) {
				for (int i = 0; i < old_archetype->chunks.size(); i++) {
					if (old_archetype->chunks[i] == chnk) {
						old_archetype->chunks[i] = backChunk;
						break;
					}
				}
			}
			old_archetype->chunks.pop_back();

			if (full(chnk))
			{
				old_archetype->full_chunks--;
			}
			old_archetype->chunklist_need_sort = true;
			//reorder_chunks(old_archetype);
			//patch chunk
			

			newArch->chunks.push_back(chnk);

			if (full(chnk))
			{
				newArch->full_chunks++;
			}
			//convert old versions to the new ones
			DataChunkHeader* old_header = chnk->header();
			DataChunkHeader* new_header = allocate_chunk_header(newArch->componentList);
			new_header->ownerArchetype = newArch;
			new_header->componentList = newArch->componentList;
			new_header->last = old_header->last;
			//TODO optimize this dumb loop
			for (int oldc = 0; oldc < old_archetype->componentList->components.size(); oldc++) {
				for (int newc = 0; newc < newArch->componentList->components.size(); newc++) {

					if (old_archetype->componentList->components[oldc].type == newArch->componentList->components[newc].type) {
						
						*new_header->version_ptr(newc) = *old_header->version_ptr(oldc);
					}
				}
			}

			delete_chunk_header(old_header);
			chnk->chunk_headers = new_header;

			newArch->chunklist_need_sort = true;
			

		}
		//adds a component to an entire chunk. If the component is a tag its a fast path
		template<typename C>
		void add_component_to_chunk(ECSWorld* world, DataChunk* chnk)
		{
			const Metatype* type = get_metatype<C>();

			assert(type->align == 0);

			Archetype* oldarch = chnk->header()->ownerArchetype;
			Archetype* newarch = make_archetype_by_adding_component(oldarch, type);
			if (oldarch != newarch) {
				change_chunk_archetype(chnk, newarch);
			}
		}
		//removes a component from an entire chunk. If the component is a tag its a fast path
		template<typename C>
		void remove_component_from_chunk(ECSWorld* world, DataChunk* chnk)
		{
			const Metatype* type = get_metatype<C>();
			assert(type->align == 0);

			Archetype* oldarch = chnk->header()->ownerArchetype;
			Archetype* newarch = make_archetype_by_removing_component(oldarch, type);
			if (oldarch != newarch) {

				change_chunk_archetype(chnk, newarch);
			}
		}

		template<typename C>
		void remove_component_from_entity(ECSWorld* world, EntityID id)
		{
			

			const Metatype* type = get_metatype<C>();

			Archetype* oldarch = get_entity_archetype(world, id);

			Archetype* newarch = make_archetype_by_adding_component(oldarch, type);

			set_entity_archetype(newarch, id);
#if 0
			const Metatype* temporalMetatypeArray[32];
			ChunkComponentList* oldlist = oldarch->componentList;
			bool typeFound = false;
			int lenght = oldlist->components.size();
			for (int i = 0; i < lenght; i++) {
				temporalMetatypeArray[i] = oldlist->components[i].type;

				//the pointers for metatypes are allways fully stable
				if (temporalMetatypeArray[i] == type) {
					typeFound = true;
					//swap last
					temporalMetatypeArray[i] = oldlist->components[lenght - 1].type;
				}
			}

			Archetype* newArch = oldarch;
			if (typeFound) {

				lenght--;
				sort_metatypes(temporalMetatypeArray, lenght);

				newArch = find_or_create_archetype(world, temporalMetatypeArray, lenght);

				set_entity_archetype(newArch, id);
			}
#endif
		}


		//template<typename A, typename B, typename C, typename D, typename Func>
		//void entity_chunk_iterate(DataChunk* chnk, Func&& function) {
		//
		//	auto array0 = get_chunk_array<A>(chnk);
		//	auto array1 = get_chunk_array<B>(chnk);
		//	auto array2 = get_chunk_array<C>(chnk);
		//	auto array3 = get_chunk_array<D>(chnk);
		//
		//	assert(array0.chunkOwner == chnk);
		//	assert(array1.chunkOwner == chnk);
		//	assert(array2.chunkOwner == chnk);
		//	assert(array3.chunkOwner == chnk);
		//	for (int i = chnk->header.last - 1; i >= 0; i--) {
		//		function(array0[i], array1[i], array2[i], array3[i]);
		//	}
		//}

		//by skypjack
		template<typename... Args, typename Func>
		void entity_chunk_iterate(DataChunk* chnk, Func&& function, uint64_t execution_id = 0) {
			auto tup = std::make_tuple(get_chunk_array<Args>(chnk)...);
#ifndef NDEBUG
			(assert(std::get<decltype(get_chunk_array<Args>(chnk))>(tup).chunkOwner == chnk), ...);
#endif
			if (execution_id != 0) {
				((std::get<decltype(get_chunk_array<Args>(chnk))>(tup).set_version(execution_id) ), ...);
			}

			for (int i = chnk->count() - 1; i >= 0; i--) {
				function(std::get<decltype(get_chunk_array<Args>(chnk))>(tup)[i]...);
			}
		}


		template<typename ...Args, typename Func>
		void unpack_chunk(type_list<Args...> types, DataChunk* chunk, Func&& function, uint64_t execution_id) {
			entity_chunk_iterate<Args...>(chunk, function, execution_id);
		}
		template<typename ...Args>
		Query& unpack_querywith(type_list<Args...> types, Query& query) {
			return query.with<Args...>();
		}





		inline int insert_entity_in_chunk(DataChunk* chunk, EntityID EID, bool bInitializeConstructors) {
			int index = -1;

			ChunkComponentList* cmpList = chunk->header()->componentList;

			if (chunk->count() < (uint32_t)cmpList->chunkCapacity) {

				index = chunk->header()->last;
				chunk->header()->last++;

				if (bInitializeConstructors) {
					//initialize component
					for (auto& cmp : cmpList->components) {
						const Metatype* mtype = cmp.type;

						if (!mtype->is_empty()) {
							void* ptr = (void*)((byte*)chunk + cmp.chunkOffset + (mtype->size * index));

							mtype->constructor(ptr);
						}
					}
				}


				//insert eid
				EntityID* eidptr = ((EntityID*)chunk);
				eidptr[index] = EID;

				//if full, reorder it on archetype
				if (chunk->count() == cmpList->chunkCapacity) {
					set_chunk_full(chunk);
				}
			}

			return index;
		}

		//returns ID of the moved entity
		inline EntityID erase_entity_in_chunk(DataChunk* chunk, uint16_t index) {

			ChunkComponentList* cmpList = chunk->header()->componentList;

			bool bWasFull = chunk->count() == cmpList->chunkCapacity;
			assert(chunk->count() > index);

			bool bPop = chunk->count() > 1 && index != (chunk->count() - 1);
			int popIndex = chunk->count() - 1;

			chunk->header()->last--;

			//clear and pop last
			for (auto& cmp : cmpList->components) {
				const Metatype* mtype = cmp.type;

				if (!mtype->is_empty()) {
					void* ptr = (void*)((byte*)chunk + cmp.chunkOffset + (mtype->size * index));

					mtype->destructor(ptr);

					if (bPop) {
						void* ptrPop = (void*)((byte*)chunk + cmp.chunkOffset + (mtype->size * popIndex));
						memcpy(ptr, ptrPop, mtype->size);
					}
				}
			}

			EntityID* eidptr = ((EntityID*)chunk);
			eidptr[index] = EntityID{};


			if (chunk->count() == 0) {
				delete_chunk_from_archetype(chunk);
			}
			else if (bWasFull) {
				set_chunk_partial(chunk);
			}

			if (bPop) {
				chunk->header()->ownerArchetype->ownerWorld->entities[eidptr[popIndex].index].chunkIndex = index;
				eidptr[index] = eidptr[popIndex];

				return eidptr[index];
			}
			else {
				return EntityID{};
			}
		}

		

		inline DataChunk* build_chunk(ChunkComponentList* cmpList) {

			DataChunk* chunk = new DataChunk();

			

			chunk->chunk_headers = allocate_chunk_header(cmpList);

			chunk->header()->last = 0;
			chunk->header()->componentList = cmpList;
			for (uint16_t i = 0; i < cmpList->components.size(); i++) {
				*chunk->version_ptr(i) = 1;
			}

			return chunk;
		}
	}

	inline ECSWorld::ECSWorld()
	{
		Archetype* nullArch = new Archetype();

		nullArch->full_chunks = 0;
		nullArch->componentList = adv::build_component_list(nullptr, 0);
		nullArch->componentHash = 0;
		nullArch->ownerWorld = this;

		archetypes.push_back(nullArch);

		archetypeSignatures.push_back(0);

		archetype_signature_map[0].push_back(nullArch);

		//we want archs to allways have 1 chunk at least, create initial
		adv::create_chunk_for_archetype(nullArch);
	}

	template<typename Container>
	int ECSWorld::gather_chunks(Query& query, Container& container)
	{
		int count = 0;
		adv::iterate_matching_archetypes(this, query, [&](Archetype* arch) {

			for (auto chnk : arch->chunks) {
				count++;
				container.push_back(chnk);
			}
			});
		return count;
	}
	template<typename Container,typename Function>
	int ECSWorld::gather_chunks_filtered(Query& query, Container& container, Function&& filter)
	{
		int count = 0;
		adv::iterate_matching_archetypes(this, query, [&](Archetype* arch) {

			for (auto chnk : arch->chunks) {
				if (filter(chnk))
				{
					count++;
					container.push_back(chnk);
				}				
			}
		});
		return count;
	}

	inline void ECSWorld::destroy(EntityID eid)
	{
		adv::destroy_entity(this, eid);
	}

	template<typename Func>
	void decs::ECSWorld::for_each(Query& query, Func&& function, uint64_t execution_id)
	{
		using params = decltype(adv::args(&Func::operator()));

		adv::iterate_matching_archetypes(this, query, [&](Archetype* arch) {

			for (auto chnk : arch->chunks) {

				adv::unpack_chunk(params{}, chnk, function, execution_id);
			}
			});
	}
	template<typename Func>
	void decs::ECSWorld::for_each(Func&& function, uint64_t execution_id)
	{
		using params = decltype(adv::args(&Func::operator()));

		Query query;
		adv::unpack_querywith(params{}, query).build();

		for_each<Func>(query, std::move(function),execution_id);
	}

	template<typename C>
	inline void ECSWorld::add_component(EntityID id, C& comp)
	{
		adv::add_component_to_entity<C>(this, id, comp);
	}

	template<typename C>
	void ECSWorld::add_component(EntityID id)
	{
		adv::add_component_to_entity<C>(this, id);
	}

	template<typename C>
	inline void ECSWorld::remove_component(EntityID id)
	{
		adv::remove_component_from_entity<C>(this, id);
	}


	template<typename C>
	bool ECSWorld::has_component(EntityID id)
	{
		return adv::has_component<C>(this, id);
	}

	template<typename C>
	C& ECSWorld::get_component(EntityID id)
	{
		return adv::get_entity_component<C>(this, id);
	}

	template<typename C>
	inline C* ECSWorld::set_singleton()
	{
		constexpr MetatypeHash type = Metatype::template build_hash<C>();

		C* old_singleton = get_singleton<C>();
		if (old_singleton) {			
			delete old_singleton;
		}
		{
			C* new_singleton = new C;
			singleton_map[type.name_hash] = (void*)new_singleton;
			return new_singleton;
		}
	}

	template<typename C>
	inline C* ECSWorld::set_singleton(C&& singleton)
	{
		constexpr MetatypeHash type = Metatype::template build_hash<C>();

		C* old_singleton = get_singleton<C>();
		if (old_singleton) {
			*old_singleton = singleton;
			return old_singleton;
		}
		else {

			C* new_singleton = new C(singleton);
			singleton_map[type.name_hash] = (void*)new_singleton;
			return new_singleton;
		}

	}

	template<typename C>
	inline C* ECSWorld::get_singleton()
	{
		constexpr MetatypeHash type = Metatype::template build_hash<C>();

		auto lookup = singleton_map.find(type.name_hash);
		if (lookup != singleton_map.end()) {
			return (C*)singleton_map[type.name_hash];
		}
		else {
			return nullptr;
		}
	}

	template<typename ...Comps>
	inline EntityID decs::ECSWorld::new_entity()
	{
		Archetype* arch = nullptr;
		//empty component list will use the hardcoded null archetype
		if constexpr (sizeof...(Comps) != 0) {
			static const Metatype* types[] = { adv::get_metatype<Comps>()... };
			constexpr size_t num = (sizeof(types) / sizeof(*types));

			adv::sort_metatypes(types, num);
			arch = adv::find_or_create_archetype(this, types, num);
		}
		else {
			arch = get_empty_archetype();
		}

		return adv::create_entity_with_archetype(arch);
	}
}


}
}