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
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	float2 UV : UV;
};

VSOut main(VSIn input)
{
	VSOut output;
	
	output.Position = input.Position;
	output.Normal = input.Normal;
	output.Tangent = input.Tangent;
	output.Bitangent = input.Bitangent;
	output.UV = input.UV;
	
	return output;
}
