#pragma once
#include <PrecompiledHeader.h>
using namespace DirectX;


// Shader resources
enum ConstanBuffer
{
	CB_Appliation,
	CB_Frame,
	CB_Object,
	NumConstantBuffers
};

// Vertex data for a colored cube.
struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

struct DXGlobals {

	
  const LONG g_WindowWidth = 1280;
  const LONG g_WindowHeight = 720;
  LPCSTR g_WindowClassName = "DirectXWindowClass";
  LPCSTR g_WindowName = "DirectX Template";
  HWND g_WindowHandle = 0;

  const BOOL g_EnableVSync = FALSE;

// Direct3D device and swap chain.
  ID3D11Device* g_d3dDevice = nullptr;
  ID3D11DeviceContext* g_d3dDeviceContext = nullptr;
  IDXGISwapChain* g_d3dSwapChain = nullptr;

// Render target view for the back buffer of the swap chain.
  ID3D11RenderTargetView* g_d3dRenderTargetView = nullptr;
// Depth/stencil view for use as a depth buffer.
  ID3D11DepthStencilView* g_d3dDepthStencilView = nullptr;
// A texture to associate to the depth stencil view.
  ID3D11Texture2D* g_d3dDepthStencilBuffer = nullptr;

// Define the functionality of the depth/stencil stages.
  ID3D11DepthStencilState* g_d3dDepthStencilState = nullptr;
// Define the functionality of the rasterizer stage.
  ID3D11RasterizerState* g_d3dRasterizerState = nullptr;
  D3D11_VIEWPORT g_Viewport = { 0 };

// Vertex buffer data
  ID3D11InputLayout* g_d3dInputLayout = nullptr;
  ID3D11Buffer* g_d3dVertexBuffer = nullptr;
  ID3D11Buffer* g_d3dIndexBuffer = nullptr;

// Shader data
  ID3D11VertexShader* g_d3dVertexShader = nullptr;
  ID3D11PixelShader* g_d3dPixelShader = nullptr;

  ID3D11Buffer* g_d3dConstantBuffers[NumConstantBuffers];


  // Demo parameters
  XMMATRIX g_WorldMatrix;
  XMMATRIX g_ViewMatrix;
  XMMATRIX g_SecondViewMatrix;
  XMMATRIX g_ProjectionMatrix;


  VertexPosColor g_CubeVertices[8] =
  {
	  { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
  { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
  { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
  { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
  { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
  { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
  { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
  { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
  };

  WORD g_CubeIndicies[36] =
  {
	  0, 1, 2, 0, 2, 3,
	  4, 6, 5, 4, 7, 6,
	  4, 5, 1, 4, 1, 0,
	  3, 2, 6, 3, 6, 7,
	  1, 5, 6, 1, 6, 2,
	  4, 0, 3, 4, 3, 7
  };
};

extern DXGlobals *Globals;

  


