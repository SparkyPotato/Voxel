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
	nointerpolation float Area : AREA;
	nointerpolation float4 Plane : PLANE;
};

[maxvertexcount(3)]
void main(triangle GSIn input[3], inout TriangleStream<GSOut> stream)
{
	float3 normal = normalize(abs(input[0].Normal + input[1].Normal + input[2].Normal));
	uint max = normal[1] > normal[0] ? 1 : 0;
	max = normal[2] > normal[max] ? 2 : max;
	
	float3 s1 = input[1].Position - input[0].Position;
	float3 s2 = input[2].Position - input[0].Position;
	float3 area = cross(s1, s2);
	float areaSquared = dot(area, area);
	
	float4 plane = float4(normal, dot(normal, input[0].Position));

	for (uint i = 0; i < 3; i++)
	{
		GSOut output;

		output.Position.xyz = (input[i].Position.xyz /*- CameraPosition*/) / VoxelHalfExtent;
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
		output.Area = areaSquared;
		output.Plane = plane;

		stream.Append(output);
	}
}
