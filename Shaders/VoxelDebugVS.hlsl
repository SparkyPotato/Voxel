#include "Voxel.hlsli"

struct VSOut
{
	float4 Position : SV_Position;
	float4 Color : COLOR;
};

VSOut main(uint id : SV_VertexID)
{
	VSOut output;
	
	uint3 coords = Unflatten(id, VoxelGridRes);
	output.Position = float4(coords, 1.f);
	output.Color = VoxelTexture.Load(int4(coords, 0));
	
	return output;
}