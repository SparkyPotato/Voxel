cbuffer ConstantBuffer : register(b0)
{
	float2 BumpMapSize;
}

SamplerState Sampler : register(s0);

Texture2D AlbedoMap : register(t0);
Texture2D SpecularMap : register(t1);
Texture2D BumpMap : register(t2);

struct PSIn
{
	float3 WorldPosition : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
	float2 UV : UV;
};

struct PSOut
{
	float4 Material : SV_Target0;
	float4 Normal : SV_Target1;
};

float3 CalcBumpMap(float3 normal, float3 tangent, float3 bitangent, float2 uv)
{
	float3x3 tangentToWorld = float3x3(tangent, normal, bitangent);
	
	float2 offset = float2(1.f, 1.f) / BumpMapSize;
	float current = BumpMap.Sample(Sampler, uv).r;
	float dx = BumpMap.Sample(Sampler, uv + float2(offset.x, 0.f)).r - current;
	float dy = BumpMap.Sample(Sampler, uv + float2(0.f, offset.y)).r - current;

	float bumpMult = -3.f;
	float3 calc = normalize(float3(bumpMult * dx, 1.f, bumpMult * dy));

	return mul(calc, tangentToWorld);
}

PSOut main(PSIn input)
{
	PSOut output;
	
	float2 uv = input.UV;
	
	float4 albedo = AlbedoMap.Sample(Sampler, uv);
	if (albedo.a < 0.5f) discard;
	float4 specular = SpecularMap.Sample(Sampler, uv);
	output.Material = float4(albedo.rgb, specular.r);
	output.Normal = float4(CalcBumpMap(input.Normal, input.Tangent, input.Bitangent, uv), 1.f);
	
	return output;
}
