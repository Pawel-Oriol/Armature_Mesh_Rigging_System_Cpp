#pragma once
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


#include "../DirectXTK/WICTextureLoader.h"
#include "../DirectXTK/DDSTextureLoader.h"
#include "d3d_wrappers.h"


void BindCrtHandlesToStdHandles(bool bindStdIn, bool bindStdOut, bool bindStdErr);

ID3D11Buffer* CreateConstantBuffer(ID3D11Device* devicePtr, unsigned char* data, size_t sz);

ID3D11Buffer* CreateVertexBuffer(ID3D11Device* devicePtr, unsigned char* data, size_t sz);