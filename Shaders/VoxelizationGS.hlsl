#include "Voxel.hlsli"

struct GSIn
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	float2 UV : UV;
};

struct GSOut
{
	float4 Position : SV_Position;
	float3 WorldPosition : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	float2 UV : UV;
};

[maxvertexcount(3)]
void main(triangle GSIn input[3], inout TriangleStream<GSOut> stream)
{
	float3 normal = abs(input[0].Normal + input[1].Normal + input[2].Normal);
	uint max = normal[1] > normal[0] ? 1 : 0;
	max = normal[2] > normal[max] ? 2 : max;

	for (uint i = 0; i < 3; i++)
	{
		GSOut output;

		output.Position.xyz = (input[i].Position.xyz/* - CameraPosition*/) / VoxelHalfExtent;
		
		[flatten]
		if (max == 0)
		{
			output.Position.xyz = output.Position.zyx;
		}
		else if (max == 1)
		{
			output.Position.xyz = output.Position.xzy;
		}

		output.Position.xy /= VoxelGridRes;
		output.Position.zw = 1;

		output.WorldPosition = input[i].Position.xyz;
		output.Normal = input[i].Normal;
		output.Tangent = input[i].Tangent;
		output.Bitangent = input[i].Bitangent;
		output.UV = input[i].UV;

		stream.Append(output);
	}
}
