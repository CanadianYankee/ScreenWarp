struct VertexIn
{
	uint2 index : VERTEXID;
	float2 PosL  : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
	float2 TexCoord : TEXCOORD;
	float4 PosH  : SV_POSITION;
};

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

struct PosVel
{
	float2 Pos;
	float2 Vel;
};

StructuredBuffer<PosVel> posVel : register(t0);

static const float PI = 3.14159265f;

VertexOut VS( VertexIn vin )
{
	VertexOut vout;
	
	switch (sc_iSaverType)
	{
	case 0: // stGridPulse
		vout.PosH = float4(
			vin.PosL.x + sin(vin.PosL.x * PI * sc_iParams.x) * sc_fParams.x * sin(fv_fTime),
			vin.PosL.y + sin(vin.PosL.y * PI * sc_iParams.y) * sc_fParams.x * sin(fv_fTime),
			0.0f, 1.0f);
		vout.TexCoord = vin.TexCoord;
		break;

	case 3: // stBreathe
		vout.PosH = float4(
			vin.PosL.x * (1.0f + sc_fParams.x * sin(sc_fParams.y * fv_fTime) * cos(vin.PosL.y * PI * 0.5)),
			vin.PosL.y * (1.0f + sc_fParams.x * sin(sc_fParams.y * fv_fTime) * cos(vin.PosL.x * PI * 0.5)),
			0.0f, 1.0f);
		vout.TexCoord = vin.TexCoord;
		break;

	case 4: // stSprings
	{
		uint i = vin.index.x + vin.index.y * (sc_iParams.x + 1);
		vout.PosH = float4(posVel[i].Pos, 0.0f, 1.0f);
		vout.TexCoord = vin.TexCoord;
	}
	break;

	default:
		vout.PosH = float4(vin.PosL, 0.0f, 1.0f);
		vout.TexCoord = vin.TexCoord;
		break;
	}
    
    return vout;
}
