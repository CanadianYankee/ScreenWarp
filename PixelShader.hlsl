texture2D g_txImage;

cbuffer cbSaverConfig : register(b0)
{
	uint sc_iSaverType;
	uint3 sc_iParams;

	float4 sc_fParams;
};

cbuffer cbFrameVariables : register(b1)
{
	float fv_fTime;
	float fv_fDt;
	uint2 fv_iParams;

	float4 fv_fParams;
};

SamplerState g_sampleTexture : register(s0);

struct PixelIn
{
	float2 TexCoord : TEXCOORD;
};

float4 PS(PixelIn pin) : SV_TARGET
{
	float4 color;

	switch (sc_iSaverType)
	{
	case 1: // stColorPuslse
		{
			float4 cTex = g_txImage.Sample(g_sampleTexture, pin.TexCoord);
			color.xyz = cTex.xyz * (1.0 - fv_fParams.w) + fv_fParams.xyz * fv_fParams.w;
			color.w = 1.0f;
		}
		break;

	default:
		color = g_txImage.Sample(g_sampleTexture, pin.TexCoord);
		break;
	}

	return color;
}
