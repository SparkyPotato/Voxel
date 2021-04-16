cbuffer ConstantBuffer : register(b0)
{
	uint Mode;
}

SamplerState Sampler : register(s0);

Texture2D Albedo : register(t0);
Texture2D Normal : register(t1);
Texture2D Depth : register(t2);

struct PSIn
{
	float2 UV : UV;
};

float4 main(PSIn input) : SV_TARGET
{
	switch (Mode)
	{
	case 0:
		return float4(Albedo.Sample(Sampler, input.UV).rgb, 1.f);
	case 1:
		return float4(Albedo.Sample(Sampler, input.UV).aaa, 1.f);
	case 2:
		return float4(Normal.Sample(Sampler, input.UV).rgb, 1.f);
	case 3:
		return float4(Depth.Sample(Sampler, input.UV).rrr, 1.f);
	default:
		return float4(1.f, 1.f, 1.f, 1.f);
	};
}
