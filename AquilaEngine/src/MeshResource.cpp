#include <PrecompiledHeader.h>

#include "MeshResource.h"



template<typename vT>
void MeshBuilder<vT>::Reserve(size_t vertexSize, size_t indexSize)
{
	BuiltMeshData.Vertices.reserve(vertexSize);
	BuiltMeshData.Indices.reserve(indexSize);
}

template<typename vT>
void MeshBuilder<vT>::AddQuad(vT a, vT b, vT c, vT d)
{
	BuiltMeshData.Vertices.push_back(a);

	BuiltMeshData.Vertices.push_back(b);

	BuiltMeshData.Vertices.push_back(c);

	BuiltMeshData.Vertices.push_back(c);

	BuiltMeshData.push_back(lastIndex + 0);
	BuiltMeshData.Indices.push_back(lastIndex + 1);
	BuiltMeshData.Indices.push_back(lastIndex + 2);

	BuiltMeshData.Indices.push_back(lastIndex + 0);
	BuiltMeshData.Indices.push_back(lastIndex + 2);
	BuiltMeshData.Indices.push_back(lastIndex + 3);

	lastIndex += 6;
}

template<typename vT>
void MeshBuilder<vT>::AddTriangle(vT a, vT b, vT c)
{
	BuiltMeshData.Vertices.push_back(a);
	BuiltMeshData.Indices.push_back(lastIndex + 0);
	

	BuiltMeshData.Vertices.push_back(b);
	BuiltMeshData.Indices.push_back(lastIndex + 1);
	

	BuiltMeshData.Vertices.push_back(c);
	BuiltMeshData.Indices.push_back(lastIndex + 2);
	
	lastIndex += 3;	
}