Texture2D<uint4> screen : register(t0);
RWTexture2D<float4> draw : register(u0);

#define blocksize 16

[numthreads(blocksize, blocksize, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
	uint4 inColor = screen[DTid.xy];

	float4 f = (inColor & 0xFF) / 255.0f;

	float4 outColor = float4(f.zyx, 1.0f);

	draw[DTid.xy] = outColor;
}
