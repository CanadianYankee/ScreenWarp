#include "stdafx.h"
#include "FullScreenSaver.h"
#include "DDSTextureLoader.h"

inline float frand()
{
	return (float)rand() / (float)RAND_MAX;
}

CFullScreenSaver::CFullScreenSaver()
{
	m_4xMsaaQuality = 2;
	m_bEnable4xMsaa = true;
	m_bSingleQuad = false;

	m_eSaverType = stSprings; // (SAVER_TYPE)(rand() % NUM_SAVER_TYPES);

	switch (m_eSaverType)
	{
		case stGridPulse:
		case stBreathe:
		case stSprings:
		m_bSingleQuad = false;
		break;

		case stColorPulse:
		case stBlur:
		m_bSingleQuad = true;
		break;
	}
}

CFullScreenSaver::~CFullScreenSaver()
{
}

BOOL CFullScreenSaver::InitSaverData()
{
	HRESULT hr = S_OK;

	hr = CreateGeometryBuffers();

	if (SUCCEEDED(hr))
	{
		hr = LoadShaders();
	}

	if (SUCCEEDED(hr))
	{
		hr = CreateShaderConstants();
	}

	if (SUCCEEDED(hr))
	{
		hr = ConvertBitmap();
	}

	return SUCCEEDED(hr);
}

void CFullScreenSaver::CleanUpSaver()
{
	if (m_pD3DContext)
	{
		m_pD3DContext->ClearState();
	}
}

BOOL CFullScreenSaver::OnResizeSaver()
{
	return TRUE;
}

void CFullScreenSaver::PopulateConfig()
{
	m_config.sc_iSaverType = (UINT)m_eSaverType;

	switch (m_eSaverType)
	{
		case stGridPulse:
		{
			// int params: (x, y) = number of cells in each direction
			// float params: x = amplitude of pulse
			m_config.sc_iParams = XMUINT3(rand() % 10 + 3, rand() % 10 + 3, 0);
			m_config.sc_fParams = XMFLOAT4(0.02f, 0.0f, 0.0f, 0.0f);
		}
		break;

		case stColorPulse:
		{
			// float params: (x, y, z, w) = angular frequency of RGBA oscillation
			m_config.sc_fParams = XMFLOAT4(frand() * 6.0f - 3.0f, frand() * 6.0f - 3.0f, frand() * 6.0f - 3.0f, frand() * 6.0f - 3.0f);
		}
		break;

		case stBlur:
		{
			// int params: (x, y) = size of draw texture
			m_config.sc_iParams = XMUINT3(m_iClientWidth, m_iClientHeight, 0);
		}
		break;

		case stBreathe:
		{
			// float params: x = amplitude; y = angular frequency
			m_config.sc_fParams = XMFLOAT4((frand() + 0.2f) * 0.3f, frand() * 0.25f + 1.0f, 0.0f, 0.0f);
		}
		break;

		case stSprings:
		{
			assert(!m_bSingleQuad);
			// int params: (x, y) = number of tiles in each direction
			// float params: (x, y) = grid spacing in each direction; z = spring constant
			int iXTiles = (m_rcScreen.right - m_rcScreen.left + m_iMeshSize - 1) / m_iMeshSize;
			int iYTiles = (m_rcScreen.bottom - m_rcScreen.top + m_iMeshSize - 1) / m_iMeshSize;
			m_config.sc_iParams = XMUINT3(iXTiles, iYTiles, 0);
			m_config.sc_fParams = XMFLOAT4(2.0f / (float)iXTiles, 2.0f / (float)iYTiles, 0.7f, 0.0f);
		}
		break;

	default:
		assert(false);
	}
}

HRESULT CFullScreenSaver::CreateGeometryBuffers()
{
	HRESULT hr = S_OK;

	// Create vertex buffer
	int iXTiles = m_bSingleQuad ? 1 : (m_rcScreen.right - m_rcScreen.left + m_iMeshSize - 1) / m_iMeshSize;
	int iYTiles = m_bSingleQuad ? 1 : (m_rcScreen.bottom - m_rcScreen.top + m_iMeshSize - 1) / m_iMeshSize;
	m_iVertexCount = (iXTiles + 1) * (iYTiles + 1);
	std::vector<Vertex> vertices(m_iVertexCount);

	// Tile the vertex buffer
	std::vector<Vertex>::iterator iVertex = vertices.begin();
	for (int j = 0; j <= iYTiles; j++)
	{
		float y = (float)j / (float)iYTiles;
		for (int i = 0; i <= iXTiles; i++)
		{
			float x = (float)i / (float)iXTiles;
			iVertex->Index = XMUINT2(i, j);
			iVertex->Pos = XMFLOAT2(2.0f * x - 1.0f, 2.0f * y - 1.0f);
			iVertex->TexCoord = XMFLOAT2(x, y);
			iVertex++;
		}
	}

	D3D11_SUBRESOURCE_DATA vinitData = { 0 };
	vinitData.pSysMem = vertices.data();
	vinitData.SysMemPitch = 0;
	vinitData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC vbd(m_iVertexCount * sizeof(Vertex), D3D11_BIND_VERTEX_BUFFER);
	hr = m_pD3DDevice->CreateBuffer(&vbd, &vinitData, &m_pScreenVB);
	if (FAILED(hr)) return hr;
	D3DDEBUGNAME(m_pScreenVB, "Screen VB");

	// Create auxillary buffers if the compute shader will do physics on the vertices
	switch (m_eSaverType)
	{
		case stSprings:
		{
			std::vector<PosVel> vecPosVel(m_iVertexCount);
			int i = 0;
			for (int y = 0; y <= iYTiles; y++)
			{
				for (int x = 0; x <= iXTiles; x++)
				{
					vecPosVel[i].Pos = vertices[i].Pos;
					vecPosVel[i].Vel = XMFLOAT2(0.1f * (frand() - 0.5f) / (float)m_iMeshSize, 0.1f * (frand() - 0.5f) / (float)m_iMeshSize);
					i++;
				}
			}

			CD3D11_BUFFER_DESC descPosVel(m_iVertexCount * sizeof(PosVel), D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT,
				0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof(PosVel));
			D3D11_SUBRESOURCE_DATA dataPosVel;
			dataPosVel.pSysMem = vecPosVel.data();
			dataPosVel.SysMemPitch = 0;
			dataPosVel.SysMemSlicePitch = 0;
			hr = m_pD3DDevice->CreateBuffer(&descPosVel, &dataPosVel, &m_pVBPosVel);
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pVBPosVel, "VB PosVel");

			hr = m_pD3DDevice->CreateUnorderedAccessView(m_pVBPosVel.Get(), nullptr, &m_pUAVVBPosVel);
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pUAVVBPosVel, "VB PosVel UAV");
			hr = m_pD3DDevice->CreateShaderResourceView(m_pVBPosVel.Get(), nullptr, &m_pSRVVBPosVel);
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pSRVVBPosVel, "VB PosVel SRV");

			hr = m_pD3DDevice->CreateBuffer(&descPosVel, nullptr, &m_pVBPosVelNext);
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pVBPosVelNext, "VB Next PosVel");

			hr = m_pD3DDevice->CreateUnorderedAccessView(m_pVBPosVelNext.Get(), nullptr, &m_pUAVVBPosVelNext);
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pUAVVBPosVelNext, "VB Next PosVel UAV");
			hr = m_pD3DDevice->CreateShaderResourceView(m_pVBPosVelNext.Get(), nullptr, &m_pSRVVBPosVelNext);
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pSRVVBPosVelNext, "VB Next PosVel SRV");
		}
		break;
	}
	vertices.clear();

	// Create the index buffer
	m_iIndexCount = iXTiles * iYTiles * 6;
	std::vector<UINT> indices(m_iIndexCount);
	std::vector<UINT>::iterator iIndex = indices.begin();
	for (int v = 0; v < iYTiles; v++)
	{
		for (int u = 0; u < iXTiles; u++)
		{
			int iBase = u + v * (iXTiles + 1);

			*iIndex = iBase;					
			iIndex++;
			*iIndex = iBase + iXTiles + 1;		
			iIndex++;
			*iIndex = iBase + 1;				
			iIndex++;

			*iIndex = iBase + 1;				
			iIndex++;
			*iIndex = iBase + iXTiles + 1;		
			iIndex++;
			*iIndex = iBase + iXTiles + 2;		
			iIndex++;
		}

	}

	D3D11_SUBRESOURCE_DATA iinitData = { 0 };
	iinitData.pSysMem = indices.data();
	iinitData.SysMemPitch = 0;
	iinitData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC ibd(m_iIndexCount * sizeof(UINT), D3D11_BIND_INDEX_BUFFER);
	hr = m_pD3DDevice->CreateBuffer(&ibd, &iinitData, &m_pScreenIB);
	if (FAILED(hr)) return hr;
	D3DDEBUGNAME(m_pScreenVB, "Screen IB");
	indices.clear();

	return hr;
}

HRESULT CFullScreenSaver::LoadShaders()
{
	HRESULT hr = S_OK;

	ComPtr<ID3D11DeviceChild> pShader;

	const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "VERTEXID", 0, DXGI_FORMAT_R32G32_UINT, 0, offsetof(Vertex, Index), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, Pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	VS_INPUTLAYOUTSETUP ILS;
	ILS.pInputDesc = vertexDesc;
	ILS.NumElements = ARRAYSIZE(vertexDesc);
	ILS.pInputLayout = NULL;
	hr = LoadShader(VertexShader, L"VertexShader.cso", nullptr, &pShader, &ILS);
	if (SUCCEEDED(hr))
	{
		hr = pShader.As<ID3D11VertexShader>(&m_pVertexShader);
		m_pInputLayout = ILS.pInputLayout;
		ILS.pInputLayout->Release();
		D3DDEBUGNAME(m_pVertexShader, "Vertex Shader");
		D3DDEBUGNAME(m_pInputLayout, "Input Layout");
	}
	if (FAILED(hr)) return hr;

	hr = LoadShader(PixelShader, L"PixelShader.cso", nullptr, &pShader);
	if (SUCCEEDED(hr))
	{
		hr = pShader.As<ID3D11PixelShader>(&m_pPixelShader);
	}
	if (FAILED(hr)) return hr;
	D3DDEBUGNAME(m_pPixelShader, "Pixel Shader");

	hr = LoadShader(ComputeShader, L"CSConvert.cso", nullptr, &pShader);
	if (SUCCEEDED(hr))
	{
		hr = pShader.As<ID3D11ComputeShader>(&m_pCSConvert);
	}
	if (FAILED(hr)) return hr;
	D3DDEBUGNAME(m_pCSConvert, "Convert CS");

	// If our saver type needs a compute shader, load it now
	switch (m_eSaverType)
	{
		case stBlur:
		{
			hr = LoadShader(ComputeShader, L"CSAdjustDraw.cso", nullptr, &pShader);
			if (SUCCEEDED(hr))
			{
				hr = pShader.As<ID3D11ComputeShader>(&m_pCSAdjustDraw);
			}
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pCSAdjustDraw, "Adjust Draw CS");
		}
		break;

		case stSprings:
		{
			hr = LoadShader(ComputeShader, L"CSAdjustVertex.cso", nullptr, &pShader);
			if (SUCCEEDED(hr))
			{
				hr = pShader.As<ID3D11ComputeShader>(&m_pCSAdjustVertex);
			}
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pCSAdjustVertex, "Adjust Vertex CS");
		}
	}

	return hr;
}

HRESULT CFullScreenSaver::CreateShaderConstants()
{
	HRESULT hr = S_OK;

	// Copy the image currently displayed on this screen to a memory DI bitmap
	int iWidth = m_rcScreen.right - m_rcScreen.left;
	int iHeight = m_rcScreen.bottom - m_rcScreen.top;
	HDC hScreenDC = GetDC(nullptr);
	HDC hMemDC = CreateCompatibleDC(hScreenDC);
	BITMAPINFOHEADER bmpInfo;
	ZeroMemory(&bmpInfo, sizeof(bmpInfo));
	bmpInfo.biSize = sizeof(bmpInfo);
	bmpInfo.biWidth = iWidth;
	bmpInfo.biHeight = iHeight;
	bmpInfo.biPlanes = 1;
	bmpInfo.biBitCount = 32;
	bmpInfo.biCompression = BI_RGB;
	HBITMAP hMemBitmap = CreateDIBitmap(hScreenDC, &bmpInfo, 0, nullptr, nullptr, 0);
	SelectObject(hMemDC, hMemBitmap);
	BOOL bRet = BitBlt(hMemDC, 0, 0, iWidth, iHeight, hScreenDC, m_rcScreen.left, m_rcScreen.top, SRCCOPY | CAPTUREBLT);
	if (bRet)
	{
		// Now get the bitmap data and copy it into a texture for DirectX use
		DWORD dwPitch = ((iWidth * 32 + 31) / 32) * 4;
		DWORD dwBmpSize = dwPitch * iHeight;
		LPVOID pBits = malloc(dwBmpSize);
		int iRet = GetDIBits(hMemDC, hMemBitmap, 0, iHeight, pBits, (BITMAPINFO *)&bmpInfo, DIB_RGB_COLORS);

		if (iRet)
		{
			CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_R8G8B8A8_UINT, iWidth, iHeight, 1, 1, D3D11_BIND_SHADER_RESOURCE);
			D3D11_SUBRESOURCE_DATA data;
			data.pSysMem = pBits;
			data.SysMemPitch = dwPitch;
			data.SysMemSlicePitch = dwBmpSize;
			hr = m_pD3DDevice->CreateTexture2D(&desc, &data, &m_pScreenTexture);
			if (SUCCEEDED(hr))
			{
				D3DDEBUGNAME(m_pScreenTexture, "Screen Texture");
				hr = m_pD3DDevice->CreateShaderResourceView(m_pScreenTexture.Get(), nullptr, &m_pSRVScreen);
				D3DDEBUGNAME(m_pScreenTexture, "Screen SRV");
			}
		}
		free(pBits);
	}
	DeleteObject(hMemBitmap);
	DeleteDC(hMemDC);
	ReleaseDC(nullptr, hScreenDC);
	if (FAILED(hr)) return hr;

	// Create the floating-point format texture for the screen capture and the necessary views
	CD3D11_TEXTURE2D_DESC drawDesc(DXGI_FORMAT_R32G32B32A32_FLOAT, iWidth, iHeight, 1, 0, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
	hr = m_pD3DDevice->CreateTexture2D(&drawDesc, nullptr, &m_pDrawTexture);
	if (FAILED(hr)) return hr;
	D3DDEBUGNAME(m_pDrawTexture, "Draw Texture");
	
	hr = m_pD3DDevice->CreateUnorderedAccessView(m_pDrawTexture.Get(), nullptr, &m_pUAVDraw);
	if (FAILED(hr)) return hr;
	D3DDEBUGNAME(m_pUAVDraw, "Draw UAV");
	hr = m_pD3DDevice->CreateShaderResourceView(m_pDrawTexture.Get(), nullptr, &m_pSRVDraw);
	if (FAILED(hr)) return hr;
	D3DDEBUGNAME(m_pSRVDraw, "Draw SRV");

	// If the compute shader needs a second draw buffer, make it now
	switch (m_eSaverType)
	{
		case stBlur:
		{
			CD3D11_TEXTURE2D_DESC drawDesc(DXGI_FORMAT_R32G32B32A32_FLOAT, iWidth, iHeight, 1, 0, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
			hr = m_pD3DDevice->CreateTexture2D(&drawDesc, nullptr, &m_pDrawNextTexture);
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pDrawNextTexture, "Draw Next Texture");

			hr = m_pD3DDevice->CreateUnorderedAccessView(m_pDrawNextTexture.Get(), nullptr, &m_pUAVDrawNext);
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pUAVDraw, "Draw Next UAV");
			hr = m_pD3DDevice->CreateShaderResourceView(m_pDrawNextTexture.Get(), nullptr, &m_pSRVDrawNext);
			if (FAILED(hr)) return hr;
			D3DDEBUGNAME(m_pSRVDraw, "Draw Next SRV");
		}
		break;
	}
	CD3D11_SAMPLER_DESC SamplerDesc;
	ZeroMemory(&SamplerDesc, sizeof(SamplerDesc));
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = m_pD3DDevice->CreateSamplerState(&SamplerDesc, &m_pTextureSampler);
	if (FAILED(hr)) return hr;
	D3DDEBUGNAME(m_pTextureSampler, "Texture Sampler");

	// Create the constant buffers
	PopulateConfig();
	D3D11_SUBRESOURCE_DATA cbData;
	cbData.pSysMem = &m_config;
	cbData.SysMemPitch = 0;
	cbData.SysMemSlicePitch = 0;
	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.ByteWidth = sizeof(SAVER_CONFIG);
	hr = m_pD3DDevice->CreateBuffer(&desc, &cbData, &m_pCBSaverConfig);
	if (FAILED(hr)) return hr;
	D3DDEBUGNAME(m_pTextureSampler, "Saver Config CB");

	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.ByteWidth = sizeof(FRAME_VARIABLES);
	hr = m_pD3DDevice->CreateBuffer(&desc, nullptr, &m_pCBFrameVariables);
	if (FAILED(hr)) return hr;
	D3DDEBUGNAME(m_pTextureSampler, "Frame Variables CB");

	return hr;
}

// The screen bitmap is integer format, need to convert to floating point for bitmap sampling to work
HRESULT CFullScreenSaver::ConvertBitmap()
{
	// Load the compute shader
	m_pD3DContext->CSSetShader(m_pCSConvert.Get(), nullptr, 0);

	// Set the input and output textures
	m_pD3DContext->CSSetShaderResources(0, 1, m_pSRVScreen.GetAddressOf());
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_pUAVDraw.GetAddressOf(), nullptr);

	// Dispatch the compute shader to do the conversion
	int iTilesX = (m_rcScreen.right - m_rcScreen.left + 15) / 16;
	int iTilesY = (m_rcScreen.bottom - m_rcScreen.top + 15) / 16;
	m_pD3DContext->Dispatch(iTilesX, iTilesY, 1);

	// Unbind the resource views
	ID3D11UnorderedAccessView* pUAVNULL[1] = { nullptr };
	ID3D11ShaderResourceView* pSRVNULL[1] = { nullptr };
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, pUAVNULL, nullptr);
	m_pD3DContext->CSSetShaderResources(0, 1, pSRVNULL);

	return S_OK;
}

BOOL CFullScreenSaver::PauseSaver()
{
	return TRUE;
}

BOOL CFullScreenSaver::ResumeSaver()
{
	return TRUE;
}

BOOL CFullScreenSaver::IterateSaver(SAVERFLOAT dt, SAVERFLOAT T)
{
	BOOL bResult = UpdateScene(dt, T);
	if (bResult)
		bResult = RenderScene();

	return bResult;
}

BOOL CFullScreenSaver::UpdateScene(float dt, float T)
{
	HRESULT hr = S_OK;

	FRAME_VARIABLES cbFrameVars = { 0 };
	cbFrameVars.fv_fTime = T;
	cbFrameVars.fv_fDt = dt;

	switch (m_eSaverType)
	{
		case stColorPulse:
		{
			float R = 0.5f * sinf(T * m_config.sc_fParams.x) + 0.5f;
			float G = 0.5f * sinf(T * m_config.sc_fParams.y) + 0.5f;
			float B = 0.5f * sinf(T * m_config.sc_fParams.z) + 0.5f;
			float A = 0.5f * sinf(T * m_config.sc_fParams.w) + 0.5f;
			cbFrameVars.fv_fParams = XMFLOAT4(R, G, B, A);
		}
		break;
	}

	// Copy the frame variables to the GPU for use by the shaders
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	hr = m_pD3DContext->Map(m_pCBFrameVariables.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (SUCCEEDED(hr))
	{
		CopyMemory(mappedResource.pData, &cbFrameVars, sizeof(FRAME_VARIABLES));
		m_pD3DContext->Unmap(m_pCBFrameVariables.Get(), 0);
	}

	// If this saver runs a compute shader, do it now
	switch (m_eSaverType)
	{
		case stBlur:
		{
			// Load the compute shader
			m_pD3DContext->CSSetShader(m_pCSAdjustDraw.Get(), nullptr, 0);

			// Set the input and output textures
			m_pD3DContext->CSSetShaderResources(0, 1, m_pSRVDraw.GetAddressOf());
			m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_pUAVDrawNext.GetAddressOf(), nullptr);

			// Set the constant buffers
			ID3D11Buffer *CBBuffers[2] = { m_pCBSaverConfig.Get(), m_pCBFrameVariables.Get() };
			m_pD3DContext->CSSetConstantBuffers(0, 2, CBBuffers);

			// Dispatch the compute shader to do the conversion
			int iTilesX = (m_rcScreen.right - m_rcScreen.left + 15) / 16;
			int iTilesY = (m_rcScreen.bottom - m_rcScreen.top + 15) / 16;
			m_pD3DContext->Dispatch(iTilesX, iTilesY, 1);

			// Unbind the resource views
			ID3D11UnorderedAccessView* pUAVNULL[1] = { nullptr };
			ID3D11ShaderResourceView* pSRVNULL[1] = { nullptr };
			m_pD3DContext->CSSetUnorderedAccessViews(0, 1, pUAVNULL, nullptr);
			m_pD3DContext->CSSetShaderResources(0, 1, pSRVNULL);

			// Swap the drawing textures
			SwapComPtr<ID3D11Texture2D>(m_pDrawTexture, m_pDrawNextTexture);
			SwapComPtr<ID3D11ShaderResourceView>(m_pSRVDraw, m_pSRVDrawNext);
			SwapComPtr<ID3D11UnorderedAccessView>(m_pUAVDraw, m_pUAVDrawNext);
		}
		break;

		case stSprings:
		{
			// Load the compute shader
			m_pD3DContext->CSSetShader(m_pCSAdjustVertex.Get(), nullptr, 0);

			// Set the input and output buffers
			m_pD3DContext->CSSetShaderResources(0, 1, m_pSRVVBPosVel.GetAddressOf());
			m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_pUAVVBPosVelNext.GetAddressOf(), nullptr);

			// Set the constant buffers
			ID3D11Buffer *CBBuffers[2] = { m_pCBSaverConfig.Get(), m_pCBFrameVariables.Get() };
			m_pD3DContext->CSSetConstantBuffers(0, 2, CBBuffers);

			// Dispatch the compute shader to do the conversion
			int iTiles = (m_iVertexCount + 255) / 256;
			m_pD3DContext->Dispatch(iTiles, 1, 1);

			// Unbind the resource views
			ID3D11UnorderedAccessView* pUAVNULL[1] = { nullptr };
			ID3D11ShaderResourceView* pSRVNULL[1] = { nullptr };
			m_pD3DContext->CSSetUnorderedAccessViews(0, 1, pUAVNULL, nullptr);
			m_pD3DContext->CSSetShaderResources(0, 1, pSRVNULL);

			// Swap the current and future pos / vel
			SwapComPtr<ID3D11Buffer>(m_pVBPosVel, m_pVBPosVelNext);
			SwapComPtr<ID3D11ShaderResourceView>(m_pSRVVBPosVel, m_pSRVVBPosVelNext);
			SwapComPtr<ID3D11UnorderedAccessView>(m_pUAVVBPosVel, m_pUAVVBPosVelNext);
		}
		break;
	}

	return TRUE;
}

BOOL CFullScreenSaver::RenderScene()
{
	HRESULT hr = S_OK;

	if(!m_pSwapChain || !m_pD3DContext) 
	{
		assert(false);
		return FALSE;
	}

	const float clrBackground[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	m_pD3DContext->ClearRenderTargetView(m_pRenderTargetView.Get(), clrBackground);
	m_pD3DContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_pD3DContext->IASetInputLayout(m_pInputLayout.Get());
	m_pD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_pD3DContext->IASetVertexBuffers(0, 1, m_pScreenVB.GetAddressOf(), &stride, &offset);
	m_pD3DContext->IASetIndexBuffer(m_pScreenIB.Get(), DXGI_FORMAT_R32_UINT, 0);

	m_pD3DContext->VSSetShader(m_pVertexShader.Get(), NULL, 0);
	ID3D11Buffer *CBBuffers[2] = { m_pCBSaverConfig.Get(), m_pCBFrameVariables.Get() };
	m_pD3DContext->VSSetConstantBuffers(0, 2, CBBuffers);
	switch (m_eSaverType)
	{
		case stSprings:
		{
			m_pD3DContext->VSSetShaderResources(0, 1, m_pSRVVBPosVel.GetAddressOf());
		}
		break;
	}
	m_pD3DContext->PSSetConstantBuffers(0, 2, CBBuffers);

	m_pD3DContext->PSSetShaderResources(0, 1, m_pSRVDraw.GetAddressOf());
	m_pD3DContext->PSSetSamplers(0, 1, m_pTextureSampler.GetAddressOf());
	m_pD3DContext->PSSetShader(m_pPixelShader.Get(), NULL, 0);

	m_pD3DContext->DrawIndexed(m_iIndexCount, 0, 0);

	hr = m_pSwapChain->Present(1, 0);

	// Unbind the resources
	ID3D11ShaderResourceView* pSRVNULL[1] = { nullptr };
	m_pD3DContext->PSSetShaderResources(0, 1, pSRVNULL);
	m_pD3DContext->VSSetShaderResources(0, 1, pSRVNULL);

	return SUCCEEDED(hr);
}
