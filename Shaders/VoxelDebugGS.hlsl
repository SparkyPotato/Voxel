#include "Voxel.hlsli"

struct GSOut
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

float3 CreateCube(uint vertexID)
{
	uint b = 1u << vertexID;
	return float3((0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0);
}

[maxvertexcount(1)]
void main(point GSOut input[1], inout PointStream<GSOut> output)
{
	[branch]
	if (input[0].Color.a > 0.f)
	{
//		for (uint i = 0; i < 14; i++)
//		{
//			GSOut element;
//			element.Position = input[0].Position;
//			element.Color = input[0].Color;
//
//			element.Position.xyz = element.Position.xyz / VoxelGridRes * 2.f - 1.f;
//			element.Position.y = -element.Position.y;
//			element.Position.xyz *= VoxelGridRes;
//			element.Position.xyz += (CreateCube(i) - float3(0, 1, 0)) * 2;
//			element.Position.xyz *= VoxelHalfExtent;
//			element.Position.xyz += CameraPosition;
//
//			element.Position = mul(float4(element.Position.xyz, 1.f), ViewProjection);
//
//			output.Append(element);
//		}
//		output.RestartStrip();
		
		GSOut element;
		element.Position = input[0].Position;
		element.Color = input[0].Color;
		element.Position.xyz = element.Position.xyz / VoxelGridRes * 2.f - 1.f;
		element.Position.y = -element.Position.y;
		element.Position.xyz *= VoxelGridRes;
		element.Position.xyz *= VoxelHalfExtent;
		// element.Position.xyz += CameraPosition;
		element.Position = mul(float4(element.Position.xyz, 1.f), ViewProjection);
		
		output.Append(element);
	}
}