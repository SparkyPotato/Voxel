Texture2D Albedo : register(t0);
Texture2D Normal : register(t1);
Texture2D Depth : register(t2);

#include "Voxel.hlsli"

struct PSIn
{
	float2 UV : UV;
};

float3 Unproject(float2 uv)
{
	float4 ndcPosition = float4(0.f, 0.f, 0.f, 1.f);
	ndcPosition.z = Depth.Sample(Sampler, uv).r;
	ndcPosition.x = uv.x * 2.f - 1.f;
	ndcPosition.y = (1 - uv.y) * 2.f - 1.f;
	
	float4 unprojected = mul(ndcPosition, ViewProjectInverse);
	return unprojected.xyz / unprojected.w;
}

float4 main(PSIn input) : SV_TARGET
{
	float4 albedo = Albedo.Sample(Sampler, input.UV);
	float3 color = albedo.rgb;
	float specular = albedo.a;
	float3 normal = Normal.Sample(Sampler, input.UV).xyz;
	float3 worldPosition = Unproject(input.UV);
	float3 view = normalize(CameraPosition - worldPosition);
	
	float NdotL = dot(normal, LightDirection);
	float3 R = reflect(-LightDirection, normal);
	
	float3 phong = float3(0.f, 0.f, 0.f);
	
	[flatten]
	if (NdotL > 0.f)
	{
		phong = NdotL * LightIntensity;
		
		float RdotV = dot(R, view);
		phong += max(0.f, RdotV);
	}
	
	return (float4(phong * LightColor, 1.f) // phong direct
			* (1.f - ConeTrace(worldPosition, normal, LightDirection, 0.f).a) // shadow
			+ ConeTraceRadiance(worldPosition, normal) // diffuse indirect
			+ ConeTraceReflection(worldPosition, normal, view, (1.f - specular)) * specular // specular
			) * float4(color, 1.f);
}