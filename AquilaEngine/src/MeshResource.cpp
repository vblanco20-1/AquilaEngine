#include "MeshResource.h"



template<typename vT>
void MeshBuilder<vT>::Reserve(size_t vertexSize, size_t indexSize)
{
	Vertices.reserve(vertexSize);
	Indices.reserve(indexSize);
}

template<typename vT>
void MeshBuilder<vT>::AddQuad(vT a, vT b, vT c, vT d)
{
	Vertices.push_back(a);

	Vertices.push_back(b);

	Vertices.push_back(c);

	Vertices.push_back(c);

	Indices.push_back(lastIndex + 0);
	Indices.push_back(lastIndex + 1);
	Indices.push_back(lastIndex + 2);

	Indices.push_back(lastIndex + 0);
	Indices.push_back(lastIndex + 2);
	Indices.push_back(lastIndex + 3);

	lastIndex += 6;
}

template<typename vT>
void MeshBuilder<vT>::AddTriangle(vT a, vT b, vT c)
{
	Vertices.push_back(a);
	Indices.push_back(lastIndex + 0);
	

	Vertices.push_back(b);
	Indices.push_back(lastIndex + 1);
	

	Vertices.push_back(c);
	Indices.push_back(lastIndex + 2);
	
	lastIndex += 3;	
}