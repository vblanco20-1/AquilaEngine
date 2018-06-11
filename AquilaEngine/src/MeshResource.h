#pragma once

#include "EngineGlobals.h"

template<typename vT>
struct MeshData {
	std::vector<vT> Vertices;
	std::vector<uint16_t> Indices;
};

template<typename vT>
class MeshBuilder {

public:

	void Reserve(size_t vertexSize, size_t indexSize);

	void AddTriangle(vT a, vT b, vT c);
	void AddQuad(vT a, vT b, vT c,vT d);

	//moves the mesh data from the builder
	 void MoveMeshData(MeshData<vT> mesh) {
		
		mesh.Indices = std::move(BuiltMeshData.Indices);
		mesh.Vertices = std::move(BuiltMeshData.Vertices);
		
	};

protected:
	MeshData<vT> BuiltMeshData;
	uint16_t lastIndex{0};
};


