struct PosVel
{
	float2 Pos;
	float2 Vel;
};

StructuredBuffer<PosVel> prevPosVel : register(t0);

RWStructuredBuffer<PosVel> nextPosVel : register(u0);


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

#define blocksize 256

inline float2 SpringForce(float2 pt0, float2 pt1, float k, float d0)
{
	float2 disp = pt1 - pt0;
	float2 force = k * (length(disp) - d0) * normalize(disp);
	return force;
}

[numthreads(blocksize, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
	switch (sc_iSaverType)
	{
		case 4:  // stSprings
		{
			uint index = DTid.x;
			uint xTiles = sc_iParams.x;
			uint yTiles = sc_iParams.y;
			float x0 = sc_fParams.x;
			float y0 = sc_fParams.y;
			float k = sc_fParams.z;

			uint x = index % (xTiles + 1);
			uint y = index / (xTiles + 1);

			float dt = fv_fDt;

			if ((x > 0) && (y > 0) && (x < xTiles) && (y < yTiles))
			{
				// Calculate the total spring force
				float2 grid0 = float2(x0, y0);
				float2 p0 = prevPosVel[index].Pos / grid0;
				float2 p1 = prevPosVel[index + 1].Pos / grid0;
				float2 p2 = prevPosVel[index - 1].Pos / grid0;
				float2 p3 = prevPosVel[index + xTiles + 1].Pos / grid0;
				float2 p4 = prevPosVel[index - (xTiles + 1)].Pos / grid0;

				float2 force = SpringForce(p0, p1, k, 0.0f) + SpringForce(p0, p2, k, 0.0f) + SpringForce(p0, p3, k, 0.0f) + SpringForce(p0, p4, k, 0.0f);
				force = force * grid0;

				// Update the position based on the stored velocity
				nextPosVel[index].Pos = prevPosVel[index].Pos + dt * prevPosVel[index].Vel;
				// Accelerate the velocity based on the calculated force
				nextPosVel[index].Vel = prevPosVel[index].Vel + dt * force;
			}
			else
			{
				nextPosVel[DTid.x] = prevPosVel[DTid.x];
			}
		}
		break;

		default:
		nextPosVel[DTid.x] = prevPosVel[DTid.x];
		break;
	}
}

