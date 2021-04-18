#include "Voxel.hlsli"

RWStructuredBuffer<Voxel> VoxelGrid : register(u0);
RWTexture3D<float4> DirectTexture : register(u1);

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
	uint flattened = Flatten(id, VoxelGridRes);
	Voxel voxel = VoxelGrid[flattened];
	
	float4 color = DecodeColor(voxel.Direct);
	
	DirectTexture[id] = color.a > 0.f ? lerp(DirectTexture[id], float4(color.rgb, 1.f), 0.2f) : 0.f;
	
	VoxelGrid[flattened].Direct = 0;
}