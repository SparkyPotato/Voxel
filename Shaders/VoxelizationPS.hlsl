#include "Voxel.hlsli"

RWStructuredBuffer<Voxel> VoxelGrid : register(u0);

Texture2D AlbedoMap : register(t0);

struct PSIn
{
	float4 Position : SV_Position;
	float3 WorldPosition : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	float2 UV : UV;
};

void main(PSIn input)
{
	float3 color = AlbedoMap.Sample(Sampler, input.UV).xyz;
	
	float NdotL = dot(input.Normal, LightDirection);
	float4 output = float4(0.f, 0.f, 0.f, 1.f);
	if (NdotL > 0.f)
	{
		float3 col = NdotL * LightIntensity * color * LightColor;
		output.xyz += col;
	}
	
	float3 diff = (input.WorldPosition /*- CameraPosition*/) / (VoxelGridRes * VoxelHalfExtent);
	float3 uvw = diff * float3(0.5f, -0.5f, 0.5f) + 0.5f;
	uint3 coords = floor(uvw * VoxelGridRes);
	
	uint id = Flatten(coords, VoxelGridRes);
	InterlockedMax(VoxelGrid[id].Direct, EncodeColor(output));
	InterlockedMax(VoxelGrid[id].Normal, EncodeNormal(input.Normal));
}
