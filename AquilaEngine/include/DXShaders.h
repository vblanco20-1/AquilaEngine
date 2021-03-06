#pragma once

#include "EngineGlobals.h"

bool LoadContent();


struct ObjectUniformStruct
{
	XMMATRIX worldMatrix[512];
	XMFLOAT4 color[512];
};


// Get the latest profile for the specified shader type.
template< class ShaderClass >
std::string GetLatestProfile();


template< class ShaderClass >
ShaderClass* CreateShader(ID3DBlob* pShaderBlob, ID3D11ClassLinkage* pClassLinkage);



template< class ShaderClass >
ShaderClass* LoadShader(const std::wstring& fileName, const std::string& entryPoint, const std::string& _profile);