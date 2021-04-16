#include "Voxel.hlsli"

RWStructuredBuffer<Voxel> VoxelGrid : register(u0);
RWTexture3D<float4> DirectTexture : register(u1);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	Voxel voxel = VoxelGrid[id.x];
	
	float4 color = DecodeColor(voxel.Direct);
	uint3 coords = Unflatten(id.x, VoxelGridRes);
	
	DirectTexture[coords] = color.a > 0.f ? lerp(DirectTexture[coords], float4(color.rgb, 1.f), 0.2f) : 0.f;
	
	VoxelGrid[id.x].Direct = 0;
}