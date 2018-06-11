#include "DXShaders.h"
DXGlobals *Globals{nullptr};
bool LoadContent()
{
	assert(Globals->g_d3dDevice);

	// Create an initialize the vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));

	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof(VertexPosColor) * _countof(Globals->g_CubeVertices);
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData;
	ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));

	resourceData.pSysMem = Globals->g_CubeVertices;

	HRESULT hr = Globals->g_d3dDevice->CreateBuffer(&vertexBufferDesc, &resourceData, &Globals->g_d3dVertexBuffer);
	if (FAILED(hr))
	{
		return false;
	}

	// Create and initialize the index buffer.
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));

	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.ByteWidth = sizeof(WORD) * _countof(Globals->g_CubeIndicies);
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	resourceData.pSysMem = Globals->g_CubeIndicies;

	hr = Globals->g_d3dDevice->CreateBuffer(&indexBufferDesc, &resourceData, &Globals->g_d3dIndexBuffer);
	if (FAILED(hr))
	{
		return false;
	}

	// Create the constant buffers for the variables defined in the vertex shader.
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.ByteWidth = sizeof(XMMATRIX);
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = Globals->g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &Globals->g_d3dConstantBuffers[CB_Appliation]);
	if (FAILED(hr))
	{
		return false;
	}
	hr = Globals->g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &Globals->g_d3dConstantBuffers[CB_Frame]);
	if (FAILED(hr))
	{
		return false;
	}

	
	//	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.ByteWidth = sizeof(ObjectUniformStruct);
	//constantBufferDesc.CPUAccessFlags = 0;
	//constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = Globals->g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &Globals->g_d3dConstantBuffers[CB_Object]);
	if (FAILED(hr))
	{
		return false;
	}

	// Load the shaders
	//Globals->g_d3dVertexShader = LoadShader<ID3D11VertexShader>( L"../data/shaders/SimpleVertexShader.hlsl", "SimpleVertexShader", "latest" );
	//Globals->g_d3dPixelShader = LoadShader<ID3D11PixelShader>( L"../data/shaders/SimplePixelShader.hlsl", "SimplePixelShader", "latest" );

	// Load the compiled vertex shader.
	ID3DBlob* vertexShaderBlob;
	//#if _DEBUG
	//	LPCWSTR compiledVertexShaderObject = L"SimpleVertexShader_d.cso";
	//#else
	LPCWSTR compiledVertexShaderObject = L"SimpleVertexShader.cso";
	//#endif

	hr = D3DReadFileToBlob(compiledVertexShaderObject, &vertexShaderBlob);
	if (FAILED(hr))
	{
		return false;
	}

	hr = Globals->g_d3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &Globals->g_d3dVertexShader);
	if (FAILED(hr))
	{
		return false;
	}

	// Create the input layout for the vertex shader.
	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor,Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor,Color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = Globals->g_d3dDevice->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &Globals->g_d3dInputLayout);
	if (FAILED(hr))
	{
		return false;
	}

	SafeRelease(vertexShaderBlob);

	// Load the compiled pixel shader.
	ID3DBlob* pixelShaderBlob;
	//#if _DEBUG
	//	LPCWSTR compiledPixelShaderObject = L"SimplePixelShader_d.cso";
	//#else
	LPCWSTR compiledPixelShaderObject = L"SimplePixelShader.cso";
	//#endif

	hr = D3DReadFileToBlob(compiledPixelShaderObject, &pixelShaderBlob);
	if (FAILED(hr))
	{
		return false;
	}

	hr = Globals->g_d3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &Globals->g_d3dPixelShader);
	if (FAILED(hr))
	{
		return false;
	}

	SafeRelease(pixelShaderBlob);

	// Setup the projection matrix.
	RECT clientRect;
	GetClientRect(Globals->g_WindowHandle, &clientRect);

	// Compute the exact client dimensions.
	// This is required for a correct projection matrix.
	float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
	float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

	Globals->g_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), clientWidth / clientHeight, 0.1f, 10000.0f);

	Globals->g_d3dDeviceContext->UpdateSubresource(Globals->g_d3dConstantBuffers[CB_Appliation], 0, nullptr, &Globals->g_ProjectionMatrix, 0, 0);

	return true;
}


template<>
ID3D11VertexShader* CreateShader<ID3D11VertexShader>(ID3DBlob* pShaderBlob, ID3D11ClassLinkage* pClassLinkage)
{
	assert(Globals->g_d3dDevice);
	assert(pShaderBlob);

	ID3D11VertexShader* pVertexShader = nullptr;
	Globals->g_d3dDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), pClassLinkage, &pVertexShader);

	return pVertexShader;
}

template<>
ID3D11PixelShader* CreateShader<ID3D11PixelShader>(ID3DBlob* pShaderBlob, ID3D11ClassLinkage* pClassLinkage)
{
	assert(Globals->g_d3dDevice);
	assert(pShaderBlob);

	ID3D11PixelShader* pPixelShader = nullptr;
	Globals->g_d3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), pClassLinkage, &pPixelShader);

	return pPixelShader;
}


template< class ShaderClass >
ShaderClass* LoadShader(const std::wstring& fileName, const std::string& entryPoint, const std::string& _profile)
{
	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	ShaderClass* pShader = nullptr;

	std::string profile = _profile;
	if (profile == "latest")
	{
		profile = GetLatestProfile<ShaderClass>();
	}

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompileFromFile(fileName.c_str(), nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), profile.c_str(),
		flags, 0, &pShaderBlob, &pErrorBlob);

	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			std::string errorMessage = (char*)pErrorBlob->GetBufferPointer();
			OutputDebugStringA(errorMessage.c_str());

			SafeRelease(pShaderBlob);
			SafeRelease(pErrorBlob);
		}

		return false;
	}

	pShader = CreateShader<ShaderClass>(pShaderBlob, nullptr);

	SafeRelease(pShaderBlob);
	SafeRelease(pErrorBlob);

	return pShader;
}


template<>
std::string GetLatestProfile<ID3D11VertexShader>()
{
	assert(Globals->g_d3dDevice);

	// Query the current feature level:
	D3D_FEATURE_LEVEL featureLevel = Globals->g_d3dDevice->GetFeatureLevel();

	switch (featureLevel)
	{
	case D3D_FEATURE_LEVEL_11_1:
	case D3D_FEATURE_LEVEL_11_0:
	{
		return "vs_5_0";
	}
	break;
	case D3D_FEATURE_LEVEL_10_1:
	{
		return "vs_4_1";
	}
	break;
	case D3D_FEATURE_LEVEL_10_0:
	{
		return "vs_4_0";
	}
	break;
	//case D3D_FEATURE_LEVEL_9_3:
	//{
	//	return "vs_4_0_level_9_3";
	//}
	//break;
	//case D3D_FEATURE_LEVEL_9_2:
	//case D3D_FEATURE_LEVEL_9_1:
	//{
	//	return "vs_4_0_level_9_1";
	//}
	//break;
	} // switch( featureLevel )

	return "";
}

template<>
std::string GetLatestProfile<ID3D11PixelShader>()
{
	assert(Globals->g_d3dDevice);

	// Query the current feature level:
	D3D_FEATURE_LEVEL featureLevel = Globals->g_d3dDevice->GetFeatureLevel();
	switch (featureLevel)
	{
	case D3D_FEATURE_LEVEL_11_1:
	case D3D_FEATURE_LEVEL_11_0:
	{
		return "ps_5_0";
	}
	break;
	case D3D_FEATURE_LEVEL_10_1:
	{
		return "ps_4_1";
	}
	break;
	case D3D_FEATURE_LEVEL_10_0:
	{
		return "ps_4_0";
	}
	break;
	//case D3D_FEATURE_LEVEL_9_3:
	//{
	//	return "ps_4_0_level_9_3";
	//}
	//break;
	//case D3D_FEATURE_LEVEL_9_2:
	//case D3D_FEATURE_LEVEL_9_1:
	//{
	//	return "ps_4_0_level_9_1";
	//}
	//break;
	}
	return "";
}

