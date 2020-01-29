#include "stdafx.h"
#include "SaverBase.h"
#include "DDSTextureLoader.h"

CSaverBase::CSaverBase(void) :
m_hMyWindow(NULL),
m_bRunning(false),
m_4xMsaaQuality(0),
m_bEnable4xMsaa(true),
m_iSaverIndex(0),
m_iNumSavers(0)
{
	TCHAR path[_MAX_PATH];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	GetModuleFileName(NULL, path, _MAX_PATH);
	_tsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
	_tmakepath_s(path, _MAX_PATH, drive, dir, NULL, NULL);
	m_strShaderPath = path;
	m_strGFXResoucePath = path;
}

CSaverBase::~CSaverBase(void)
{
}

BOOL CSaverBase::Initialize(const CWindowCluster::SAVER_WINDOW &myWindow, int iSaverIndex, int iNumSavers)
{
	HRESULT hr = S_OK;

	if (!myWindow.hWnd)
	{
		assert(false);
		return FALSE;
	}

	m_hMyWindow = myWindow.hWnd;
	m_iSaverIndex = iSaverIndex;
	m_iNumSavers = iNumSavers;
	m_iClientWidth = myWindow.sClientArea.cx;
	m_iClientHeight = myWindow.sClientArea.cy;
	m_rcScreen = myWindow.rcScreenRect;
	m_fAspectRatio = (float)m_iClientWidth / (float)m_iClientHeight;
	m_bFullScreen = myWindow.bSaverMode;

	m_Timer.Reset();

	hr = InitDirect3D();

	BOOL bSuccess = SUCCEEDED(hr);
	if (bSuccess)
	{
		bSuccess = InitSaverData();
		if (bSuccess)
			m_bRunning = true;
	}

	m_Timer.Reset();

	return bSuccess;
}

// Create D3D device, context, and swap chain
HRESULT CSaverBase::InitDirect3D()
{
	HRESULT hr = S_OK;

	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	ComPtr<IDXGIAdapter> pAdapter;
	hr = GetMyAdapter(&pAdapter);
	if (SUCCEEDED(hr))
	{
		D3D_FEATURE_LEVEL featureLevel;
		hr = D3D11CreateDevice(pAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL,
			createDeviceFlags, NULL, 0, D3D11_SDK_VERSION, &m_pD3DDevice, &featureLevel,
			&m_pD3DContext);

		if (SUCCEEDED(hr))
		{
			if (featureLevel != D3D_FEATURE_LEVEL_11_0)
				hr = E_FAIL;
		}
		assert(SUCCEEDED(hr));
	}

	if (SUCCEEDED(hr))
	{
		// Check 4X MSAA quality support for our back buffer format.
		// All Direct3D 11 capable devices support 4X MSAA for all render 
		// target formats, so we only need to check quality support.
		hr = m_pD3DDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m_4xMsaaQuality);
		assert(SUCCEEDED(hr) && m_4xMsaaQuality > 0);
	}

	if (SUCCEEDED(hr))
	{
		if (FAILED(hr)) return hr;

		// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.
		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferDesc.Width = m_iClientWidth;
		sd.BufferDesc.Height = m_iClientHeight;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		// Use 4X MSAA? 
		if (m_bEnable4xMsaa && m_4xMsaaQuality > 0)
		{
			sd.SampleDesc.Count = 4;
			sd.SampleDesc.Quality = m_4xMsaaQuality - 1;
		}
		// No MSAA
		else
		{
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
		}

		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;
		sd.OutputWindow = m_hMyWindow;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;

		ComPtr<IDXGIFactory> dxgiFactory;
		hr = pAdapter->GetParent(__uuidof(IDXGIFactory), &dxgiFactory);

		if (SUCCEEDED(hr))
		{
			hr = dxgiFactory->CreateSwapChain(m_pD3DDevice.Get(), &sd, &m_pSwapChain);
		}

		if (SUCCEEDED(hr))
		{
			hr = dxgiFactory->MakeWindowAssociation(m_hMyWindow, DXGI_MWA_NO_WINDOW_CHANGES);
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = OnResize() ? S_OK : E_FAIL;
	}

	return hr;
}

// Look up the DXGI adapter based on the hosting HWND
HRESULT CSaverBase::GetMyAdapter(IDXGIAdapter **ppAdapter)
{
	HRESULT hr = S_OK;
	*ppAdapter = nullptr;
	bool bFound = false;

	assert(m_hMyWindow);
	HMONITOR hMonitor = MonitorFromWindow(m_hMyWindow, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFOEX monitorInfo;
	ZeroMemory(&monitorInfo, sizeof(monitorInfo));
	monitorInfo.cbSize = sizeof(monitorInfo);
	GetMonitorInfo(hMonitor, &monitorInfo);

	ComPtr<IDXGIAdapter> pAdapter;
	ComPtr<IDXGIFactory> pFactory;
	hr = CreateDXGIFactory(__uuidof(IDXGIFactory), &pFactory);
	if (SUCCEEDED(hr))
	{
		for (UINT iAdapter = 0; !bFound; iAdapter++)
		{
			// When this call fails, there are no more adapters left
			HRESULT hrIter = pFactory->EnumAdapters(iAdapter, &pAdapter);
			if (FAILED(hrIter)) break;


			for (UINT iOutput = 0; !bFound; iOutput++)
			{
				ComPtr<IDXGIOutput> pOutput;
				hrIter = pAdapter->EnumOutputs(iOutput, &pOutput);
				if (FAILED(hrIter)) break;

				DXGI_OUTPUT_DESC outputDesc;
				hr = pOutput->GetDesc(&outputDesc);
				if (SUCCEEDED(hr) && (_tcscmp(outputDesc.DeviceName, monitorInfo.szDevice) == 0))
				{
					bFound = true;
					*ppAdapter = pAdapter.Detach();
				}
			}
		}
	}

	if (SUCCEEDED(hr))
		return bFound ? S_OK : E_FAIL;
	else
		return hr;
}

HRESULT CSaverBase::LoadShader(ShaderType type, const std::wstring &strFileName, ComPtr<ID3D11ClassLinkage> pClassLinkage, ComPtr<ID3D11DeviceChild> *pShader, VS_INPUTLAYOUTSETUP *pILS)
{
	HRESULT hr = S_OK;

	std::wstring strFullPath = m_strShaderPath + strFileName;

	std::vector<char> buffer;
	hr = LoadBinaryFile(strFullPath, buffer);

	if (SUCCEEDED(hr))
	{
		switch (type)
		{
		case VertexShader:
		{
			ComPtr<ID3D11VertexShader> pVertexShader;
			hr = m_pD3DDevice->CreateVertexShader(buffer.data(), buffer.size(), pClassLinkage.Get(), &pVertexShader);
			if (SUCCEEDED(hr) && (pILS))
			{
				hr = m_pD3DDevice->CreateInputLayout(pILS->pInputDesc, pILS->NumElements,
					buffer.data(), buffer.size(), &pILS->pInputLayout);
			}
			if (SUCCEEDED(hr))
			{
				hr = pVertexShader.As<ID3D11DeviceChild>(pShader);
			}
			break;
		}

		case PixelShader:
		{
			ComPtr<ID3D11PixelShader> pPixelShader;
			hr = m_pD3DDevice->CreatePixelShader(buffer.data(), buffer.size(), pClassLinkage.Get(), &pPixelShader);
			if (SUCCEEDED(hr))
			{
				hr = pPixelShader.As<ID3D11DeviceChild>(pShader);
			}
			break;
		}

		case GeometryShader:
		{
			ComPtr<ID3D11GeometryShader> pGeometryShader;
			hr = m_pD3DDevice->CreateGeometryShader(buffer.data(), buffer.size(), pClassLinkage.Get(), &pGeometryShader);
			if (SUCCEEDED(hr))
			{
				hr = pGeometryShader.As<ID3D11DeviceChild>(pShader);
			}
			break;
		}

		case ComputeShader:
		{
			ComPtr<ID3D11ComputeShader> pComputeShader;
			hr = m_pD3DDevice->CreateComputeShader(buffer.data(), buffer.size(), pClassLinkage.Get(), &pComputeShader);
			if (SUCCEEDED(hr))
			{
				hr = pComputeShader.As<ID3D11DeviceChild>(pShader);
			}
			break;
		}

		case HullShader:
		{
			ComPtr<ID3D11HullShader> pHullShader;
			hr = m_pD3DDevice->CreateHullShader(buffer.data(), buffer.size(), pClassLinkage.Get(), &pHullShader);
			if (SUCCEEDED(hr))
			{
				hr = pHullShader.As<ID3D11DeviceChild>(pShader);
			}
			break;
		}

		case DomainShader:
		{
			ComPtr<ID3D11DomainShader> pDomainShader;
			hr = m_pD3DDevice->CreateDomainShader(buffer.data(), buffer.size(), pClassLinkage.Get(), &pDomainShader);
			if (SUCCEEDED(hr))
			{
				hr = pDomainShader.As<ID3D11DeviceChild>(pShader);
			}
			break;
		}
		}
	}

	return hr;
}

HRESULT CSaverBase::LoadTexture(const std::wstring &strFileName, ComPtr<ID3D11Texture2D> *ppTexture, ComPtr<ID3D11ShaderResourceView> *ppSRVTexture)
{
	HRESULT hr = S_OK;

	std::wstring strFullPath = m_strGFXResoucePath + strFileName;

	std::vector<char> buffer;
	hr = LoadBinaryFile(strFullPath, buffer);

	if (ppTexture) *ppTexture = nullptr;
	if (ppSRVTexture) *ppSRVTexture = nullptr;
	ComPtr<ID3D11Resource> pResource;
	ComPtr<ID3D11ShaderResourceView> pSRV;
	uint8_t *pData = reinterpret_cast<uint8_t *>(buffer.data());
	hr = CreateDDSTextureFromMemory(m_pD3DDevice.Get(), pData, buffer.size(), &pResource, &pSRV);
	if (SUCCEEDED(hr))
	{
		if (ppSRVTexture)
			pSRV.As<ID3D11ShaderResourceView>(ppSRVTexture);
		if (ppTexture)
			pResource.As<ID3D11Texture2D>(ppTexture);
	}

	return hr;
}

HRESULT CSaverBase::LoadBinaryFile(const std::wstring &strPath, std::vector<char> &buffer)
{
	HRESULT hr = S_OK;

	std::ifstream fin(strPath, std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	if (size > 0)
	{
		fin.seekg(0, std::ios_base::beg);
		buffer.resize(size);
		fin.read(buffer.data(), size);
	}
	else
	{
		hr = E_FAIL;
	}
	fin.close();

	return hr;
}

void CSaverBase::Tick()
{
	if (m_bRunning)
	{
		m_Timer.Tick();

		if (!IterateSaver(m_Timer.DeltaTime(), m_Timer.TotalTime()))
		{
			PostMessage(m_hMyWindow, WM_DESTROY, 0, 0);
		}
	}
	else
	{
	}
}

void CSaverBase::Pause()
{
	if (m_bRunning)
	{
		if (!PauseSaver())
		{
			PostMessage(m_hMyWindow, WM_DESTROY, 0, 0);
		}
		m_bRunning = false;
		m_Timer.Stop();
	}
}

void CSaverBase::Resume()
{
	if (!m_bRunning)
	{
		if (!ResumeSaver())
		{
			PostMessage(m_hMyWindow, WM_DESTROY, 0, 0);
		}
		m_bRunning = true;
		m_Timer.Start();
	}
}


void CSaverBase::ResumeResized(int cx, int cy)
{
	if (m_bRunning)
		Pause();

	m_iClientWidth = cx;
	m_iClientHeight = cy;
	m_fAspectRatio = (float)m_iClientWidth / (float)m_iClientHeight;

	if (OnResize())
	{
		Resume();
	}
	else
	{
		PostMessage(m_hMyWindow, WM_DESTROY, 0, 0);
	}
}

// Create/recreate the depth/stencil view and render target based on new size
BOOL CSaverBase::OnResize()
{
	HRESULT hr = S_OK;

	if (m_pSwapChain)
	{
		// Release the old views, as they hold references to the buffers we
		// will be destroying.  Also release the old depth/stencil buffer.
		m_pRenderTargetView = nullptr;
		m_pDepthStencilView = nullptr;
		m_pDepthStencilBuffer = nullptr;

		// Resize the swap chain and recreate the render target view.
		hr = m_pSwapChain->ResizeBuffers(1, m_iClientWidth, m_iClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

		if (SUCCEEDED(hr))
		{
			ComPtr<ID3D11Texture2D> pBackBuffer;
			hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
			if (SUCCEEDED(hr))
			{
				hr = m_pD3DDevice->CreateRenderTargetView(pBackBuffer.Get(), 0, &m_pRenderTargetView);
			}
		}

		if (SUCCEEDED(hr))
		{
			// Create the depth/stencil buffer and view.
			D3D11_TEXTURE2D_DESC depthStencilDesc;

			depthStencilDesc.Width = m_iClientWidth;
			depthStencilDesc.Height = m_iClientHeight;
			depthStencilDesc.MipLevels = 1;
			depthStencilDesc.ArraySize = 1;
			depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

			// Use 4X MSAA? --must match swap chain MSAA values.
			if (m_bEnable4xMsaa)
			{
				depthStencilDesc.SampleDesc.Count = 4;
				depthStencilDesc.SampleDesc.Quality = m_4xMsaaQuality - 1;
			}
			// No MSAA
			else
			{
				depthStencilDesc.SampleDesc.Count = 1;
				depthStencilDesc.SampleDesc.Quality = 0;
			}

			depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
			depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			depthStencilDesc.CPUAccessFlags = 0;
			depthStencilDesc.MiscFlags = 0;

			hr = m_pD3DDevice->CreateTexture2D(&depthStencilDesc, 0, &m_pDepthStencilBuffer);
			if (SUCCEEDED(hr))
			{
				hr = m_pD3DDevice->CreateDepthStencilView(m_pDepthStencilBuffer.Get(), 0, &m_pDepthStencilView);
			}
		}

		if (SUCCEEDED(hr))
		{
			// Bind the render target view and depth/stencil view to the pipeline.
			m_pD3DContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());

			// Set the viewport transform.
			m_ScreenViewport.TopLeftX = 0;
			m_ScreenViewport.TopLeftY = 0;
			m_ScreenViewport.Width = static_cast<float>(m_iClientWidth);
			m_ScreenViewport.Height = static_cast<float>(m_iClientHeight);
			m_ScreenViewport.MinDepth = 0.0f;
			m_ScreenViewport.MaxDepth = 1.0f;

			m_pD3DContext->RSSetViewports(1, &m_ScreenViewport);
		}
	}

	assert(SUCCEEDED(hr));

	return OnResizeSaver();
}

void CSaverBase::CleanUp()
{
	if (m_bRunning)
		Pause();

	CleanUpSaver();
}


