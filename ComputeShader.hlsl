Texture2D<float4> prev : register(t0);
RWTexture2D<float4> next : register(u0);

cbuffer cbSaverConfig : register(b0)
{
	uint sc_iSaverType;
	uint3 sc_iParams;

	float4 sc_fParams;
};

cbuffer cbFrameVariables : register(b1)
{
	float fv_fTime;
	uint3 fv_iParams;

	float4 fv_fParams;
};

#define blocksize 16

[numthreads(blocksize, blocksize, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
	switch (sc_iSaverType)
	{
		case 2:  // stBlur
		next[DTid.xy] = prev[DTid.xy] * 0.996 + (prev[uint2(DTid.x - 1, DTid.y)] + prev[uint2(DTid.x + 1, DTid.y)] + prev[uint2(DTid.x, DTid.y - 1)] + prev[uint2(DTid.x, DTid.y + 1)]) * 0.001f;
		break;

		default:
			next[DTid.xy] = prev[DTid.xy];
		break;
	}
}

