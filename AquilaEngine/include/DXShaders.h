#pragma once

//#include "EngineGlobals.h"
#include <DirectXMath.h>;
bool LoadContent();


struct ObjectUniformStruct
{
	DirectX::XMMATRIX worldMatrix[512];
	DirectX::XMFLOAT4 color[512];
};

