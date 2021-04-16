#include "Voxel.hlsli"

RWStructuredBuffer<Voxel> VoxelGrid : register(u0);
RWTexture3D<float4> DirectTexture : register(u1);

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
	float4 emission = VoxelTexture.Load(int4(id, 0));
	
	if (emission.a > 0.f)
	{
		float3 normal = DecodeNormal(VoxelGrid[Flatten(id, VoxelGridRes)].Normal);
		
		float3 P = ((float3) id + 0.5f) / VoxelGridRes;
		P = P * 2 - 1;
		P.y *= -1;
		P *= VoxelHalfExtent;
		P *= VoxelGridRes;
		// P += CameraPosition;
		
		float4 radiance = ConeTraceRadiance(P, normal);

		DirectTexture[id] = emission + float4(radiance.rgb, 0);
	}
	else
	{
		DirectTexture[id] = 0.f;
	}
}