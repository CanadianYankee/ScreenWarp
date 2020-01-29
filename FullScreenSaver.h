#pragma once

#include "SaverBase.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct Vertex
{
	XMUINT2 Index;
	XMFLOAT2 Pos;
	XMFLOAT2 TexCoord;
};

struct PosVel
{
	XMFLOAT2 Pos;
	XMFLOAT2 Vel;
};

class CFullScreenSaver :
	public CSaverBase
{
public:
	CFullScreenSaver();
	virtual ~CFullScreenSaver();

	enum SAVER_TYPE : UINT
	{
		stGridPulse,
		stColorPulse,
		stBlur,
		stBreathe,
		stSprings,
		NUM_SAVER_TYPES
	};

protected:
	virtual BOOL InitSaverData();
	virtual BOOL IterateSaver(SAVERFLOAT dt, SAVERFLOAT T);
	virtual BOOL PauseSaver();
	virtual BOOL ResumeSaver();
	virtual BOOL OnResizeSaver();
	virtual void CleanUpSaver();

	struct SAVER_CONFIG
	{
		UINT sc_iSaverType;
		XMUINT3 sc_iParams;

		XMFLOAT4 sc_fParams;
	};

	struct FRAME_VARIABLES
	{
		float fv_fTime;
		float fv_fDt;
		XMUINT2 fv_iParams;

		XMFLOAT4 fv_fParams;
	};

protected:
	virtual BOOL UpdateScene(float dt, float T);
	virtual BOOL RenderScene();

	HRESULT CreateGeometryBuffers();
	HRESULT LoadShaders();
	HRESULT CreateShaderConstants();
	HRESULT ConvertBitmap();
	void PopulateConfig();

	template <class T> void SwapComPtr(ComPtr<T> &ptr1, ComPtr<T> &ptr2)
	{
		ComPtr<T> temp = ptr1;  ptr1 = ptr2;  ptr2 = temp;
	}

	ComPtr<ID3D11VertexShader> m_pVertexShader;
	ComPtr<ID3D11PixelShader> m_pPixelShader;
	ComPtr<ID3D11ComputeShader> m_pCSAdjustDraw;
	ComPtr<ID3D11ComputeShader> m_pCSAdjustVertex;
	ComPtr<ID3D11ComputeShader> m_pCSConvert;
	ComPtr<ID3D11InputLayout> m_pInputLayout;

	ComPtr<ID3D11Buffer> m_pScreenVB;
	ComPtr<ID3D11Buffer> m_pScreenIB;

	ComPtr<ID3D11Buffer> m_pVBPosVel;
	ComPtr<ID3D11Buffer> m_pVBPosVelNext;
	ComPtr<ID3D11ShaderResourceView> m_pSRVVBPosVel;
	ComPtr<ID3D11ShaderResourceView> m_pSRVVBPosVelNext;
	ComPtr<ID3D11UnorderedAccessView> m_pUAVVBPosVel;
	ComPtr<ID3D11UnorderedAccessView> m_pUAVVBPosVelNext;

	ComPtr<ID3D11Buffer> m_pCBSaverConfig;
	ComPtr<ID3D11Buffer> m_pCBFrameVariables;

	ComPtr<ID3D11Texture2D> m_pScreenTexture;
	ComPtr<ID3D11Texture2D> m_pDrawTexture;
	ComPtr<ID3D11Texture2D> m_pDrawNextTexture;

	ComPtr<ID3D11ShaderResourceView> m_pSRVScreen;
	ComPtr<ID3D11UnorderedAccessView> m_pUAVDraw;
	ComPtr<ID3D11ShaderResourceView> m_pSRVDraw;
	ComPtr<ID3D11UnorderedAccessView> m_pUAVDrawNext;
	ComPtr<ID3D11ShaderResourceView> m_pSRVDrawNext;

	ComPtr<ID3D11SamplerState> m_pTextureSampler;

	BOOL m_bSingleQuad;
	const int m_iMeshSize = 8;	// Number of pixels per tile in the mesh
	UINT m_iVertexCount;
	UINT m_iIndexCount;

	SAVER_TYPE m_eSaverType;
	SAVER_CONFIG m_config;
};

