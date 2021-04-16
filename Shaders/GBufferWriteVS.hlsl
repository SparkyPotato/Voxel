cbuffer ConstantBuffer : register(b0)
{
	float4x4 ViewProjection;
	float4x4 ViewProjectInverse;
}

struct VSIn
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	float2 UV : UV;
};

struct VSOut
{
	float3 WorldPosition : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	float2 UV : UV;
	float4 Position : SV_Position;
};

VSOut main(VSIn input)
{
	VSOut output;
	output.WorldPosition = input.Position;
	output.Position = mul(float4(input.Position, 1.f), ViewProjection);
	output.Normal = input.Normal;
	output.Tangent = input.Tangent;
	output.Bitangent = input.Bitangent;
	output.UV = input.UV;
	
	return output;
}
