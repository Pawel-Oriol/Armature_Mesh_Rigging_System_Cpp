//Copyright © 2023 by Pawel Oriol

//A C++ Implementation of The Armature and Mesh Rigging System from scratch.
//A blender file of the used 3D animated model and blender expoters writeen in python by the author are also included in this project.
//You can watch the demo video of this project under the following adress :
//https://www.youtube.com/watch?v=yJpmEfbqp9k&t=47s

//Uses Direct3D11and DirectXTK(both properties of Microsoft Corp - Visit the DirectXTK subfolder for the respective the license) :
//https://github.com/microsoft/DirectXTK
// 
//Model& Animation :
//Megan & Walkcycle2 obtained from mixamo.com :
//https ://www.mixamo.com/#/?page=1&query=walk&type=Character
//https://www.youtube.com/watch?v=yJpmEfbqp9k


//Controls: Mouse + WSAD keys
//Display modes: 
//F1 - Toggle wire/solid mode
//F2 - Toggle texture/single color
//F3 - Toggle hide/show mesha
//F4 - Toggle hide/show armatury

//windows sdk libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")



#ifdef _DEBUG
#pragma comment(lib, "DirectXTK/DirectXTK_debug.lib")
#else
#pragma comment(lib, "DirectXTK/DirectXTK.lib")
#endif


#pragma comment (lib, "dinput8.lib")

#pragma comment (lib, "dxguid.lib")
#pragma comment(lib, "dxgi")

#define no_init_all deprecated

#include <time.h>
#include <iostream>
#include <cstdio>
#include <io.h>
#include<fcntl.h>
#include <dinput.h>
#include <windows.h>
#include <d3d11.h>
#include <vector>

#include <d3dcompiler.h>
#include <DirectXPackedVector.h>
#include <DirectXMath.h>
#include <dxgidebug.h>

using namespace DirectX;

#include "../DirectXTK/WICTextureLoader.h"
#include "../DirectXTK/DDSTextureLoader.h"
#include "d3d_wrappers.h"
#include "3D_lib.h"


#include <chrono>
#include <thread>

extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}


//COM objects
ID3D11Device* Device;
ID3D11DeviceContext* DevCon;
IDXGISwapChain* SwapChain;
ID3D11RenderTargetView* renderTargetView;
ID3D11DepthStencilView* depthStencilView;
ID3D11Texture2D* depthStencilBuffer;
D3D11_VIEWPORT gameViewport;


ID3D11Debug* debug;


struct AdapterAndOutputs
{
	IDXGIAdapter1* adapterPtr;
	DXGI_ADAPTER_DESC adapterDesc;
	std::vector<IDXGIOutput*> outputsList;
	std::vector<DXGI_OUTPUT_DESC> outputsDescList;
};

std::vector<AdapterAndOutputs> adapterAndOutputsList;

IDXGIFactory1* pFactory = NULL;

void InitAdaptersList()
{
	CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory);


	IDXGIAdapter1* adapterTemp;
	for (UINT i = 0; pFactory->EnumAdapters1(i, &adapterTemp) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapterAndOutputsList.push_back(AdapterAndOutputs());
		adapterAndOutputsList.back().adapterPtr = adapterTemp;
	}

	for (int ai = 0; ai < adapterAndOutputsList.size(); ai++)
	{
		adapterAndOutputsList[ai].adapterPtr->GetDesc(&adapterAndOutputsList[ai].adapterDesc);
	}
	for (int ai = 0; ai < adapterAndOutputsList.size(); ai++)
	{
		int oi = 0;
		IDXGIOutput* outputTemp;
		while (adapterAndOutputsList[ai].adapterPtr->EnumOutputs(oi, &outputTemp) != DXGI_ERROR_NOT_FOUND)
		{
			adapterAndOutputsList[ai].outputsList.push_back(outputTemp);
			++oi;
		}
	}
	DXGI_OUTPUT_DESC outputDescTemp;
	for (int ai = 0; ai < adapterAndOutputsList.size(); ai++)
	{
		for (int oi = 0; oi < adapterAndOutputsList[ai].outputsList.size(); oi++)
		{
			adapterAndOutputsList[ai].outputsList[oi]->GetDesc(&outputDescTemp);
			adapterAndOutputsList[ai].outputsDescList.push_back(outputDescTemp);
		}
	}


}

RECT GetPrimaryMonitorSize()
{
	return adapterAndOutputsList[0].outputsDescList[0].DesktopCoordinates;
}


LPCSTR WndClassName = "my_window_class";
HWND hwnd = NULL;





//direct input
IDirectInputDevice8* DIKeyboard;
IDirectInputDevice8* DIMouse;

DIMOUSESTATE mouseLastState;
LPDIRECTINPUT8 DirectInput;

bool WIREFRAME = true;
bool TEXTURED = false;
bool HIDE_MESH = false;
bool HIDE_ARMATURE = false;

ID3D11RasterizerState* rasterStateBasic;
ID3D11RasterizerState* rasterStateNoCulling;
ID3D11RasterizerState* rasterStateWireframe;

ID3D11BlendState* blendState;


ID3D11Buffer* cbufferTransformations;
ID3D11Buffer* cbufferLight;

ID3D11SamplerState* TexSamplerState;




XMMATRIX camView;
XMMATRIX camProjection;

XMVECTOR camPosition;
XMVECTOR camTarget;
XMVECTOR camUp;


XMFLOAT3 camPos;
XMFLOAT2 camAngles;


Object3D body;
Object3D shirt;
Object3D pants;
Object3D sneakers;
Object3D eyeslashes;
Object3D hair;

Armature armature;


int SCR_WIDTH_WINDOWED = 1000;
int SCR_HEIGHT_WINDOWED = 1000;

int SCR_WIDTH_WINDOWED_MIN = 800;
int SCR_HEIGHT_WINDOWED_MIN = 800;

int SCR_WIDTH_FS = 1920;
int SCR_HEIGHT_FS = 1080;

int SCR_WIDTH = SCR_WIDTH_WINDOWED;
int SCR_HEIGHT = SCR_WIDTH_WINDOWED;



bool InitializeDirect3D(HINSTANCE hInstance);
bool InitDirectInput(HINSTANCE hInstance);
void DetectInput();
void ReleaseAll();
bool InitScene();
void UpdateScene();
void DrawScene();

bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	int width,
	int height,
	bool windowed);

int messageloop();



LRESULT CALLBACK WndProc(HWND hwnd_arg,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);




struct Light
{
	XMFLOAT3 dir;
	float brightness;
};

Light lights[2];

struct Transformations
{
	XMMATRIX WVP;
	XMMATRIX World;
};




D3D11_INPUT_ELEMENT_DESC layout3D[] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0 ,D3D11_INPUT_PER_VERTEX_DATA,0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12 ,D3D11_INPUT_PER_VERTEX_DATA,0},
	{"NORMALS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20 ,D3D11_INPUT_PER_VERTEX_DATA,0},
};

UINT numElements3D = ARRAYSIZE(layout3D);
ID3D11InputLayout* vertLayout3D;


//wrapper do shaderow
class D3DShader
{
public:
	ID3D11VertexShader* VS;
	ID3D11PixelShader* PS;
	ID3D10Blob* VSBuffer;
	ID3D10Blob* PSBuffer;


	void CreateShaderFile(const wchar_t* filename)
	{
		ID3D10Blob* ppErrorMsgs;
		char* error_ptr = NULL;

		HRESULT hr = D3DCompileFromFile(filename, 0, 0, "VS", "vs_4_0", 0, 0, &this->VSBuffer, &ppErrorMsgs);
		if (hr == E_FAIL)
		{
			error_ptr = (char*)ppErrorMsgs->GetBufferPointer();
			FILE* file = fopen("VS_error.txt", "w");
			fprintf(file, "%s\n", error_ptr);
			fclose(file);
		}

		hr = D3DCompileFromFile(filename, 0, 0, "PS", "ps_4_0", 0, 0, &this->PSBuffer, &ppErrorMsgs);
		if (hr == E_FAIL)
		{
			error_ptr = (char*)ppErrorMsgs->GetBufferPointer();
			FILE* file = fopen("PS_error.txt", "w");
			fprintf(file, "%s\n", error_ptr);
			fclose(file);
		}

		hr = Device->CreateVertexShader(this->VSBuffer->GetBufferPointer(), this->VSBuffer->GetBufferSize(), NULL, &this->VS);
		hr = Device->CreatePixelShader(this->PSBuffer->GetBufferPointer(), this->PSBuffer->GetBufferSize(), NULL, &this->PS);

		if (ppErrorMsgs != NULL)
		{
			ppErrorMsgs->Release();
		}

	}

	void Release()
	{
		this->VS->Release();
		this->PS->Release();
		this->VSBuffer->Release();
		this->PSBuffer->Release();
	}

	LPVOID GetBufferPointerVS()
	{
		return this->VSBuffer->GetBufferPointer();
	}

	SIZE_T GetBufferSizeVS()
	{
		return this->VSBuffer->GetBufferSize();
	}


	LPVOID GetBufferPointerPS()
	{
		return this->PSBuffer->GetBufferPointer();
	}

	SIZE_T GetBufferSizePS()
	{
		return this->PSBuffer->GetBufferSize();
	}

	void Use()
	{
		DevCon->VSSetShader(this->VS, 0, 0);
		DevCon->PSSetShader(this->PS, 0, 0);
	}

};


D3DShader shader3D;
D3DShader shader3DTextured;

void SetWindowedMode()
{
	LONG lExStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME;
	LONG_PTR  retval = SetWindowLongPtrA(hwnd, GWL_STYLE, lExStyle);


	RECT wr; //Widow Rectangle
	wr.left = 100; //tymczasowo na szytwno
	wr.top =  100;
	wr.right = SCR_WIDTH_WINDOWED + wr.left;
	wr.bottom = SCR_HEIGHT_WINDOWED + wr.top;
	AdjustWindowRect(&wr, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

	SetWindowPos(hwnd, HWND_TOPMOST, wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top, SWP_FRAMECHANGED);

	SetForegroundWindow(hwnd);
	SetActiveWindow(hwnd);
	ShowWindow(hwnd, 1);
}


void SetFullscreenMode()
{

	LONG lExStyle = WS_POPUP | WS_VISIBLE;
	LONG_PTR  retval = SetWindowLongPtrA(hwnd, GWL_STYLE, lExStyle);
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, SCR_WIDTH_FS, SCR_HEIGHT_FS, SWP_FRAMECHANGED);

	SetForegroundWindow(hwnd);
	SetActiveWindow(hwnd);


	ShowWindow(hwnd, 1);
}


void ReleaseDInput()
{
	DIKeyboard->Release();
	DIMouse->Release();
}

void ReleaseGlobalCBuffers()
{
	cbufferTransformations->Release();
	cbufferLight->Release();
}

void ReleaseStates()
{
	DevCon->OMSetRenderTargets(0, 0, 0);
	DevCon->PSSetSamplers(0, 0, NULL);
	DevCon->VSSetConstantBuffers(0, 0, NULL);
	DevCon->PSSetConstantBuffers(0, 0, NULL);
	DevCon->RSSetState(NULL);
	rasterStateBasic->Release();
	rasterStateNoCulling->Release();
	rasterStateWireframe->Release();
	blendState->Release();
	TexSamplerState->Release();
}

void ReleaseShaders()
{

	shader3D.Release();
	shader3DTextured.Release();

}

void ReleaseLayouts()
{
	vertLayout3D->Release();
}

void ReleaseObjects3D()
{
	 body.ReleaseD3D();
	 shirt.ReleaseD3D();
	 pants.ReleaseD3D();
	 sneakers.ReleaseD3D();
	 eyeslashes.ReleaseD3D();
	 hair.ReleaseD3D();
	 armature.ReleaseD3D();
}

void ReleaseDirect3DCOMObjects()
{
	
	renderTargetView->Release();
	depthStencilView->Release();
	depthStencilBuffer->Release();
	renderTargetView->Release();
	depthStencilView->Release();
	Device->Release();
	DevCon->Release();
	SwapChain->Release();
}

void ReleaseAll()
{
	ReleaseDInput();
	ReleaseGlobalCBuffers();
	ReleaseStates();
	ReleaseShaders();
	ReleaseLayouts();
	ReleaseObjects3D();
}



int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	//We must inform The Compositing Window Manager, that we shall decide on the resolution of our window
	//and not him!
	bool dpiAware = SetProcessDPIAware();

	InitAdaptersList();

	RECT wrTemp = GetPrimaryMonitorSize();

	SCR_WIDTH_FS = wrTemp.right - wrTemp.left;
	SCR_HEIGHT_FS = wrTemp.bottom - wrTemp.top;

	//w trybie okienkowym tworzymy okno kwadratowe o 80% wysokosci aktualnej rozdzielczosci desktopowej
	SCR_HEIGHT_WINDOWED = (wrTemp.bottom - wrTemp.top) * 0.8;
	SCR_WIDTH_WINDOWED = SCR_HEIGHT_WINDOWED;


	SCR_HEIGHT_WINDOWED_MIN = (wrTemp.bottom - wrTemp.top) * 0.8;
	SCR_WIDTH_WINDOWED_MIN = SCR_HEIGHT_WINDOWED_MIN;

	SCR_WIDTH = SCR_WIDTH_WINDOWED;
	SCR_HEIGHT = SCR_HEIGHT_WINDOWED;

	srand(time(NULL));

	clock_t start_time = clock();

	if (!InitializeWindow(hInstance, nShowCmd, SCR_WIDTH, SCR_HEIGHT, false))
	{
		MessageBox(0, "Window Initialization has failed", "Error", MB_OK);
		return 0;
	}
	clock_t end_time = clock();

#ifdef EDIT_STUFF
	printf("InitializeWindow time %d\n", end_time - start_time);
#endif
	start_time = clock();
	if (!InitializeDirect3D(hInstance))
	{
		MessageBox(0, "Direct3D Initialization has failed", "Error", MB_OK);
		return 0;
	}
	end_time = clock();
#ifdef EDIT_STUFF
	printf("InitializeDirect3D time %d\n", end_time - start_time);
#endif
	start_time = clock();
	

	end_time = clock();
#ifdef EDIT_STUFF
	printf("InitializeXaudio2 time %d\n", end_time - start_time);
#endif
	start_time = clock();
	if (!InitScene())
	{
		MessageBox(0, "Scene Initialization has failed", "Error", MB_OK);
		return 0;
	}
	end_time = clock();
#ifdef EDIT_STUFF
	printf("InitScene time %d\n", end_time - start_time);
#endif

	start_time = clock();
	if (!InitDirectInput(hInstance))
	{
		MessageBox(0, "Direct3D Initialization has faile", "Error", MB_OK);
		return 0;
	}
	end_time = clock();
#ifdef EDIT_STUFF
	printf("InitDirectInput time %d\n", end_time - start_time);
#endif
	messageloop();

	ReleaseAll();

	return 0;
}


bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	int width,
	int height,
	bool windowed)
{

#ifdef EDIT_STUFF
	BindCrtHandlesToStdHandles(true, true, true);
#endif

	
	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WndClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Failed to register window class", "Error", MB_OK | MB_ICONERROR);
		return false;
	}

	RECT wr; //Widow Rectangle
	wr.left = 100; //na razie 'na sztywno'
	wr.top = 100;
	wr.right = width + wr.left;
	wr.bottom = height + wr.top;




	AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME, FALSE);
	hwnd = CreateWindowEx(NULL,
		WndClassName,
		"My Armature & Mesh Rigging Demo",
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME,
		wr.left, wr.top,
		wr.right - wr.left, wr.bottom - wr.top,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hwnd)
	{
		MessageBox(NULL, "Failed tocreate window", "Error", MB_OK | MB_ICONERROR);
		return false;
	}

	//ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);

	return true;
}

bool InitializeDirect3D(HINSTANCE hInstance)
{
	DXGI_MODE_DESC bufferDesc;

	//Deskryptor buffora
	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

	bufferDesc.Width = SCR_WIDTH;
	bufferDesc.Height = SCR_HEIGHT;
	bufferDesc.RefreshRate.Numerator = 60;
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;

	//deskryptor SwapChaina
	DXGI_SWAP_CHAIN_DESC swapChainDesc;

	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferDesc = bufferDesc;
	swapChainDesc.SampleDesc.Count = 8;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	//swapChainDesc.Flags =  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	//w debug dobrze miec wlaczony poglebiony debug - nieraz bledy w DirectX sa niezwykle "silent"
	//i znalezienie przyczyny bez poglebionej diagnozy jest wrecz niemozliwe
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	//utwórz swapchaina,  device i devicecontext
	D3D11CreateDeviceAndSwapChain(adapterAndOutputsList[0].adapterPtr, D3D_DRIVER_TYPE_UNKNOWN, NULL, creationFlags, NULL, NULL,
		D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &Device, NULL, &DevCon);

#if defined(_DEBUG)
	Device->QueryInterface(&debug);
	debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif

	//wyciagnij backbuffer
	ID3D11Texture2D* backBuffer;
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);



	//utworz renderTargerView
	Device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);

	backBuffer->Release();

	//stworzy buffor depth i stencil
	D3D11_TEXTURE2D_DESC depthStencilDesc;

	ZeroMemory(&depthStencilDesc, sizeof(D3D11_TEXTURE2D_DESC));

	depthStencilDesc.Width = SCR_WIDTH;
	depthStencilDesc.Height = SCR_HEIGHT;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	depthStencilDesc.SampleDesc.Count = 8;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	Device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);
	Device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView);



	//ustaw nasz RenderTarget
	DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);



	ZeroMemory(&gameViewport, sizeof(D3D11_VIEWPORT));

	gameViewport.TopLeftX = 0;
	gameViewport.TopLeftY = 0;
	gameViewport.Width = SCR_WIDTH;
	gameViewport.Height = SCR_HEIGHT;
	gameViewport.MinDepth = 0.0f;
	gameViewport.MaxDepth = 1.0f;

	DevCon->RSSetViewports(1, &gameViewport);

	D3D11_RASTERIZER_DESC rasterDesc;

	ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_BACK;


	HRESULT hr = Device->CreateRasterizerState(&rasterDesc, &rasterStateBasic);

	rasterDesc.CullMode = D3D11_CULL_NONE;
	hr = Device->CreateRasterizerState(&rasterDesc, &rasterStateNoCulling);

	

	rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterDesc.CullMode = D3D11_CULL_NONE;

	hr = Device->CreateRasterizerState(&rasterDesc, &rasterStateWireframe);
	

	DevCon->RSSetState(rasterStateBasic);
	
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));

	D3D11_RENDER_TARGET_BLEND_DESC rtbd;
	
	ZeroMemory(&rtbd, sizeof(rtbd));

	rtbd.BlendEnable = true;
	rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;

	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.RenderTarget[0] = rtbd;
	
	Device->CreateBlendState(&blendDesc, &blendState);


	SetWindowedMode();


	shader3D.CreateShaderFile(L"shaders/shader_3d.fx");
	shader3DTextured.CreateShaderFile(L"shaders/shader_3d_textured.fx");

	hr = Device->CreateInputLayout(layout3D, numElements3D, shader3D.GetBufferPointerVS(), shader3D.GetBufferSizeVS(), &vertLayout3D);
	DevCon->IASetInputLayout(vertLayout3D);
	DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cbufferTransformations = CreateConstantBuffer(Device, NULL, sizeof(Transformations));
	cbufferLight = CreateConstantBuffer(Device, NULL, sizeof(Light)*2);


	camProjection = XMMatrixPerspectiveFovLH(0.25f * 3.1415, (float)SCR_WIDTH / SCR_HEIGHT, 0.03f, 10000.0f);


	//Tymaczasowo. Na Clean Code przyjdzie pora.
	lights[0].brightness = 0.1;
	lights[0].dir.x = 0.0;
	lights[0].dir.y = 0.0;
	lights[0].dir.z = 1.0;

	lights[1].brightness = 0.5;
	lights[1].dir.x = 1.0;
	lights[1].dir.y = 1.0;
	lights[1].dir.z = 1.0;


	return true;
}




BYTE keyboardStatePrev[256];



bool InitDirectInput(HINSTANCE hInstance)
{
	
	HRESULT hr = DirectInput8Create(hInstance,
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&DirectInput,
		NULL);

	hr = DirectInput->CreateDevice(GUID_SysKeyboard, &DIKeyboard, NULL);
	hr = DirectInput->CreateDevice(GUID_SysMouse, &DIMouse, NULL);

	hr = DIKeyboard->SetDataFormat(&c_dfDIKeyboard);
	hr = DIKeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	hr = DIMouse->SetDataFormat(&c_dfDIMouse);
	hr = DIMouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);



	return true;
}

void DetectInput()
{
	//auto start = std::chrono::system_clock::now();

	//clock_t input_time_end = clock();



	DIMOUSESTATE mouseCurrState;
	BYTE keyboardState[256];

	if (DIERR_OTHERAPPHASPRIO == DIMouse->Acquire() ||
		DIERR_OTHERAPPHASPRIO == DIKeyboard->Acquire())
	{
		return;
	}

	DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseCurrState);
	DIKeyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);



	/////////////////////KEYBOARD INPUT/////////////////////////////



	/////////////////////MOUSE INPUT/////////////////////////////

	BYTE left_click_temp = mouseLastState.rgbButtons[0];
	mouseLastState = mouseCurrState;


	camAngles.x += mouseCurrState.lY * 0.001;
	camAngles.y += mouseCurrState.lX * 0.001;


	XMVECTOR forward_local = XMVectorSet(0, 0, 0.3,0);


	XMVECTOR vec_x = XMVectorSet(1, 0, 0, 0);
	XMVECTOR vec_y = XMVectorSet(0, 1, 0, 0);

	//Kamera w modelu Eulerowskim, alez bez trzeciej rotacji (bez obrotu wokol osi pokrywajacej sie z kierunkiem patrzenia)
	//Jako ze przedmiotem tej demonstracji jest zaimplemetnowanie od zera systemu armatury/riga, to przynajmniej tutaj
	//skorzystamy z pomocy jaka nam oferuje DXMath i nie bedziemy calkiem wynajdywac kola od nowa
	XMMATRIX rot_x = XMMatrixRotationAxis(vec_x, camAngles.x);
	XMMATRIX rot_y = XMMatrixRotationAxis(vec_y, camAngles.y);


	XMVECTOR forward_trans = XMVector4Transform (forward_local, rot_x);
	forward_trans = XMVector4Transform(forward_trans, rot_y);

	if ((keyboardState[DIK_W] & 0x80))
	{

		camPos.x += XMVectorGetX(forward_trans);
		camPos.y += XMVectorGetY(forward_trans);
		camPos.z += XMVectorGetZ(forward_trans);

	}
	if ((keyboardState[DIK_S] & 0x80))
	{

		camPos.x -= XMVectorGetX(forward_trans);
		camPos.y -= XMVectorGetY(forward_trans);
		camPos.z -= XMVectorGetZ(forward_trans);

	}



	if ((keyboardState[DIK_F1] & 0x80) && !(keyboardStatePrev[DIK_F1] & 0x80))
		WIREFRAME = !WIREFRAME;
	
	if ((keyboardState[DIK_F2] & 0x80) && !(keyboardStatePrev[DIK_F2] & 0x80))
		TEXTURED = !TEXTURED;
	
	if ((keyboardState[DIK_F3] & 0x80) && !(keyboardStatePrev[DIK_F3] & 0x80))
		HIDE_MESH = !HIDE_MESH;
	
	if ((keyboardState[DIK_F4] & 0x80) && !(keyboardStatePrev[DIK_F4] & 0x80))
		HIDE_ARMATURE = !HIDE_ARMATURE;
	
	memcpy(keyboardStatePrev, keyboardState, sizeof(keyboardStatePrev));

}




bool InitScene()
{
	  
	//tutaj tez "na sztywno" - do poprawy w przyszlosci
	camPos.x = -1.228866;
	camPos.y = 0.765643;
	camPos.z = 2.368459;
	 
	camAngles.x = 0.048000;
	camAngles.y = 2.946001;


	D3D11_SAMPLER_DESC samplerDesc;

	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = Device->CreateSamplerState(&samplerDesc, &TexSamplerState);


	//tutaj ladujemy armature
	armature.Load(Device,"models/megan/armature.txt", "models/bone.obj");

	
	//a tutaj meshe i wagi
	body.Load(Device, "models/megan/body.obj", true, "models/megan/vertex_groups_body.txt");

	//duzo razy wczytamy te sama texture, ale na potrzeby dydaktyczne niech juz tak zostanie
	//optymalizacja nie zawsze przeklada sie na czytelnosc
	body.LoadTexture(Device, DevCon, L"models/megan/body_texture.jpg");

	//metoda pomocnicza, aby skojarzyc vertexy z transformujacymi je koscmi poprzez indexy
	//-wydajniejsze niz poprzez stringi
	armature.AssignBoneIndicesToVertexGroups(&body);

	//analogicznie reszta modeli
	shirt.Load(Device, "models/megan/shirt.obj", true, "models/megan/vertex_groups_shirt.txt");
	shirt.LoadTexture(Device, DevCon, L"models/megan/body_texture.jpg");
	armature.AssignBoneIndicesToVertexGroups(&shirt);


	pants.Load(Device, "models/megan/pants.obj", true, "models/megan/vertex_groups_pants.txt");
	pants.LoadTexture(Device, DevCon, L"models/megan/body_texture.jpg");
	armature.AssignBoneIndicesToVertexGroups(&pants);


	sneakers.Load(Device, "models/megan/sneakers.obj", true, "models/megan/vertex_groups_sneakers.txt");
	sneakers.LoadTexture(Device, DevCon, L"models/megan/body_texture.jpg");
	armature.AssignBoneIndicesToVertexGroups(&sneakers);

	eyeslashes.Load(Device, "models/megan/eyelashes.obj", true, "models/megan/vertex_groups_eyelashes.txt");
	eyeslashes.LoadTexture(Device, DevCon, L"models/megan/hair_texture.png");
	armature.AssignBoneIndicesToVertexGroups(&eyeslashes);

	hair.Load(Device, "models/megan/hair.obj", true, "models/megan/vertex_groups_hair.txt");
	hair.LoadTexture(Device, DevCon, L"models/megan/hair_texture.png");
	armature.AssignBoneIndicesToVertexGroups(&hair);

	return true;
}

void UpdateScene()
{
	//zeby sie nie zgubic w scenie
	//printf("cam pos : %f %f %f\n", camPos.x, camPos.y, camPos.z);
	//printf("cam angles : %f %f\n", camAngles.x, camAngles.y);
	

	//aktualizacja aktualnej klatki - mozemy regulowac tempo argumentem 'progress'
	armature.Animate(0.65);

	//liczenie aktualnego basis - opis szczegolowy w metodzie
	armature.ComputeCurrBasis();

	//liczymy finalne transformaty kosci - opis szczegolowy w metodzie
	armature.ComputeFinalOrientationPos();
	
	//transformacja/deformacja mesha przez armature
	armature.MeshDeform(&body);

	//musimy na nowo policzyc wektory normalne - mesh zmienil ksztalt
	//algorytm identyczny co w Blenderze - im wiekszy kat miedzy krawedziami
	//wychodzacymi z vertexa tym wiekszy wplyw trojkowa na koncowa wartosc wektora normalnego
	body.RecalculateNormals();

	armature.MeshDeform(&shirt);
	shirt.RecalculateNormals();

	armature.MeshDeform(&pants);
	pants.RecalculateNormals();

	armature.MeshDeform(&sneakers);
	sneakers.RecalculateNormals();

	armature.MeshDeform(&eyeslashes);
	eyeslashes.RecalculateNormals();

	armature.MeshDeform(&hair);
	hair.RecalculateNormals();
}


void DrawScene()
{


	float color_white[] = { 1.0f, 1.0f, 1.0f };
	float color_grey[] = { 0.03f, 0.03f, 0.03f };
	float color_black[] = { 0.0f, 0.0f, 0.0f };
	float color_red[] = { 1.0f, 0.0f, 0.0f };
	float color_green[] = { 0.0f, 1.0f, 0.0f };
	float color_blue[] = { 0.0f, 0.0f, 1.0f };
	float color_yellow[] = { 1.0f, 1.0f, 0.0f };
	float color_purple[] = { 1.0f, 0.0f, 1.0f };

	DirectX::XMFLOAT4 bgColor(0.0f, 0.005f * 50, 0.007f * 50, 1.0f);
	DevCon->ClearRenderTargetView(renderTargetView, &bgColor.x);
	DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0.0);

	

	XMVECTOR vec_x = XMVectorSet(1,0,0,0);
	XMVECTOR vec_y = XMVectorSet(0, 1, 0, 0);


	XMMATRIX rot_x = XMMatrixRotationAxis(vec_x, -camAngles.x);
	XMMATRIX rot_y = XMMatrixRotationAxis(vec_y, -camAngles.y);


	Transformations trans;
	trans.World = XMMatrixIdentity();
	trans.WVP = XMMatrixTranslation(-camPos.x,-camPos.y,-camPos.z) * rot_y* rot_x * camProjection;
	trans.WVP = XMMatrixTranspose(trans.WVP);


	DevCon->PSSetSamplers(0, 1, &TexSamplerState);
	DevCon->UpdateSubresource(cbufferTransformations, 0, NULL, &trans.WVP, 0, 0);
	DevCon->VSSetConstantBuffers(0, 1, &cbufferTransformations);

	DevCon->UpdateSubresource(cbufferLight, 0, NULL, &lights[0], 0, 0);
	DevCon->PSSetConstantBuffers(0, 1, &cbufferLight);
	

	if (WIREFRAME)
	{
		DevCon->RSSetState(rasterStateWireframe);
		
	}
	else
	{

	}
	if (TEXTURED)

		shader3DTextured.Use();

	else
		shader3D.Use();
	
	if (!HIDE_MESH)
	{
		DevCon->PSSetShaderResources(0, 1, &body.objTexture);
		body.DrawObject(DevCon);
		shirt.DrawObject(DevCon);
		pants.DrawObject(DevCon);
		sneakers.DrawObject(DevCon);

		DevCon->OMSetBlendState(blendState,NULL, 0xffffffff);
		DevCon->RSSetState(rasterStateNoCulling);
		DevCon->PSSetShaderResources(0, 1, &eyeslashes.objTexture);
		eyeslashes.DrawObject(DevCon);
		hair.DrawObject(DevCon);
		DevCon->OMSetBlendState(0, 0, 0xffffffff);
	}


	if (!HIDE_ARMATURE)
	{
		shader3D.Use();
		DevCon->RSSetState(rasterStateBasic);
		armature.DrawFinal(DevCon);
	}

	SwapChain->Present(1, 0);

	

}

clock_t frame_time = 0;
clock_t start_time = 0;
int messageloop()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));


	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);

			DispatchMessage(&msg);

		}

		else
		{
				clock_t start_time_total = clock();
				
				DetectInput();
				UpdateScene();
				DrawScene();
			
				clock_t end_time_total = clock();

				
				printf("total time: %d\n", end_time_total - start_time_total);


		}

	}

	return (int)msg.wParam;
}

bool entryResize = false;

LRESULT CALLBACK WndProc(HWND hwnd_arg,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{

	switch (msg)
	{

	case WM_SYSKEYDOWN:
		if (wParam == VK_F10 || wParam == VK_MENU)
			return 0;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			//if (MessageBox(0,
			//	"Do you really want to quit?",
			//	"Sure?",
			//	MB_YESNO | MB_ICONQUESTION) == IDYES)
			//	DestroyWindow(hwnd_arg);

			DestroyWindow(hwnd_arg);
		}

		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = SCR_WIDTH_WINDOWED_MIN;
		lpMMI->ptMinTrackSize.y = SCR_HEIGHT_WINDOWED_MIN;
		int aaa = 666;
	}
	case WM_NCCALCSIZE:
	{
#ifdef EDIT_STUFF
		printf("WM_NCCALCSIZE \n");
#endif
		break;
	}
	case WM_CLOSE:
	{
		
		break;
	}
	case WM_SIZE:
	{
#ifdef EDIT_STUFF
		printf("WM_SIZE wparam: %d\n", wParam);
#endif
		if (wParam == SIZE_MINIMIZED)
		{
#ifdef EDIT_STUFF
			printf("SIZE_MINIMIZED\n");
#endif
			
		}
		else
		{
#ifdef EDIT_STUFF
			printf("GAME RESTORED\n");
#endif
			
		}
		if (!entryResize)
		{
			if (SwapChain)
			{
				DevCon->OMSetRenderTargets(0, 0, 0);
				renderTargetView->Release();
				SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

				ID3D11Texture2D* buffer;
				SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer);
				Device->CreateRenderTargetView(buffer, NULL, &renderTargetView);
				D3D11_TEXTURE2D_DESC text_desc;
				buffer->GetDesc(&text_desc);
				SCR_WIDTH = text_desc.Width;
				SCR_HEIGHT = text_desc.Height;
#ifdef EDIT_STUFF
				printf("width: %d height: %d\n", SCR_WIDTH, SCR_HEIGHT);
#endif
				buffer->Release();


				//stworzy buffor depth i stencil
				if (true)
				{
					depthStencilView->Release();
					depthStencilBuffer->Release();
					D3D11_TEXTURE2D_DESC depthStencilDesc;

					ZeroMemory(&depthStencilDesc, sizeof(D3D11_TEXTURE2D_DESC));

					depthStencilDesc.Width = SCR_WIDTH;
					depthStencilDesc.Height = SCR_HEIGHT;
					depthStencilDesc.MipLevels = 1;
					depthStencilDesc.ArraySize = 1;
					depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
					depthStencilDesc.SampleDesc.Count = 8;
					depthStencilDesc.SampleDesc.Quality = 0;
					depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
					depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
					depthStencilDesc.CPUAccessFlags = 0;
					depthStencilDesc.MiscFlags = 0;

					Device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);
					Device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView);
				}


				//ustaw nasz RenderTarget
				DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);



				ZeroMemory(&gameViewport, sizeof(D3D11_VIEWPORT));
				gameViewport.TopLeftX = 0;
				gameViewport.TopLeftY = 0;
				gameViewport.Width = text_desc.Width;
				gameViewport.Height = text_desc.Height;
				gameViewport.MinDepth = 0.0f;
				gameViewport.MaxDepth = 1.0f;

				DevCon->RSSetViewports(1, &gameViewport);
				camProjection = XMMatrixPerspectiveFovLH(0.25f * 3.1415, (float)SCR_WIDTH / SCR_HEIGHT, 0.03f, 10000.0f);

				
				

			}
		}

		break;
	}
	case WM_ENTERSIZEMOVE:
	{
#ifdef EDIT_STUFF
		printf("skaluje\n");
#endif
		entryResize = true;
		break;
	}
	case WM_EXITSIZEMOVE:
	{
#ifdef EDIT_STUFF
		printf("juz nie skaluje\n");
#endif
		if (SwapChain)
		{
			DevCon->OMSetRenderTargets(0, 0, 0);
			renderTargetView->Release();
			SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

			ID3D11Texture2D* buffer;
			SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer);
			Device->CreateRenderTargetView(buffer, NULL, &renderTargetView);
			D3D11_TEXTURE2D_DESC text_desc;
			buffer->GetDesc(&text_desc);
			SCR_WIDTH = text_desc.Width;
			SCR_HEIGHT = text_desc.Height;
			SCR_WIDTH_WINDOWED = SCR_WIDTH;
			SCR_HEIGHT_WINDOWED = SCR_HEIGHT;

			buffer->Release();


			//stworzy buffor depth i stencil
			if (true)
			{
				depthStencilView->Release();
				depthStencilBuffer->Release();
				D3D11_TEXTURE2D_DESC depthStencilDesc;

				ZeroMemory(&depthStencilDesc, sizeof(D3D11_TEXTURE2D_DESC));

				depthStencilDesc.Width = SCR_WIDTH;
				depthStencilDesc.Height = SCR_HEIGHT;
				depthStencilDesc.MipLevels = 1;
				depthStencilDesc.ArraySize = 1;
				depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				depthStencilDesc.SampleDesc.Count = 8;
				depthStencilDesc.SampleDesc.Quality = 0;
				depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
				depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
				depthStencilDesc.CPUAccessFlags = 0;
				depthStencilDesc.MiscFlags = 0;

				Device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);
				Device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView);
			}


			//ustaw nasz RenderTarget
			DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);



			ZeroMemory(&gameViewport, sizeof(D3D11_VIEWPORT));
			gameViewport.TopLeftX = 0;
			gameViewport.TopLeftY = 0;
			gameViewport.Width = text_desc.Width;
			gameViewport.Height = text_desc.Height;
			gameViewport.MinDepth = 0.0f;
			gameViewport.MaxDepth = 1.0f;

			DevCon->RSSetViewports(1, &gameViewport);
			camProjection = XMMatrixPerspectiveFovLH(0.25f * 3.1415, (float)SCR_WIDTH / SCR_HEIGHT, 0.03f, 10000.0f);

		}
		entryResize = false;
		break;
	}
	}


	return DefWindowProc(hwnd_arg, msg, wParam, lParam);

}