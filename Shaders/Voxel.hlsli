struct Voxel
{
	uint Direct;
	uint Normal;
};

Texture3D<float4> VoxelTexture : register(t3);
Texture2D ShadowMap : register(t4);

SamplerState Sampler : register(s0);
SamplerState ShadowMapSampler : register(s1);

cbuffer ConstantBuffer : register(b0)
{
	float4x4 ViewProjection;
	float4x4 ViewProjectInverse;
}

cbuffer ConstantBuffer : register(b1)
{
	float3 CameraPosition;
	float VoxelHalfExtent;
	float VoxelGridRes;
}

cbuffer ConstantBuffer : register(b2)
{
	float3 LightDirection;
	float LightIntensity;
	float3 LightColor;
}

cbuffer ConstantBuffer : register(b3)
{
	float4x4 ToLight;
	float4x4 FromLight;
}

uint Flatten(uint3 coords, uint dimensions)
{
	return (coords.z * dimensions * dimensions) + (coords.y * dimensions) + coords.x;
}

uint3 Unflatten(uint id, uint dimensions)
{
	uint dim2 = dimensions * dimensions;
	const uint z = id / (dim2);
	id -= (z * dim2);
	const uint y = id / dimensions;
	const uint x = id % dimensions;
	return uint3(x, y, z);
}

uint EncodeColor(float4 color)
{
	float hdr = length(color.rgb);
	color.rgb /= hdr;

	uint3 iColor = uint3(color.rgb * 255.0f);
	uint iHDR = (uint) (saturate(hdr / 10.f) * 127);
	uint colorMask = (iHDR << 24u) | (iColor.r << 16u) | (iColor.g << 8u) | iColor.b;

	uint iAlpha = (color.a > 0 ? 1u : 0u);
	colorMask |= iAlpha << 31u;

	return colorMask;
}

float4 DecodeColor(uint colorMask)
{
	float hdr;
	float4 color;

	hdr = (colorMask >> 24u) & 0x0000007f;
	color.r = (colorMask >> 16u) & 0x000000ff;
	color.g = (colorMask >> 8u) & 0x000000ff;
	color.b = colorMask & 0x000000ff;

	hdr /= 127.0f;
	color.rgb /= 255.0f;

	color.rgb *= hdr * 10.f;

	color.a = (colorMask >> 31u) & 0x00000001;

	return color;
}

uint EncodeNormal(float area, float3 normal)
{
	int3 iNormal = int3(normal * 255.f);
	uint3 normalSigns;
	normalSigns.x = (iNormal.x > 0);
	normalSigns.y = (iNormal.y > 0) << 1;
	normalSigns.z = (iNormal.z > 0) << 2;
	iNormal = abs(iNormal);
	
	uint mask = normalSigns.x | normalSigns.y | normalSigns.z | (iNormal.x << 3) | (iNormal.y << 11) | (iNormal.z << 19);
	uint areaMask = (area / (VoxelHalfExtent * 2.f)) * 32;
	mask |= areaMask << 27;
	
	return mask;
}

float3 DecodeNormal(uint mask)
{
	int3 iNormal;
	iNormal.x = (mask >> 3) & 0x000000ff;
	iNormal.y = (mask >> 11) & 0x000000ff;
	iNormal.z = (mask >> 19) & 0x000000ff;
	
	int3 normalSigns = int3(mask & 1, (mask >> 1) & 1, (mask >> 2) & 1);
	normalSigns += normalSigns;
	normalSigns -= int3(-1, -1, -1);
	
	return (float3(iNormal) / 255.f) * normalSigns;
}

bool IsSaturated(float3 a)
{
	return a.x == saturate(a.x) && a.y == saturate(a.y) && a.z == saturate(a.z);
}

static const float3 CONES[] =
{
	float3(0.57735, 0.57735, 0.57735),
	float3(0.57735, -0.57735, -0.57735),
	float3(-0.57735, 0.57735, -0.57735),
	float3(-0.57735, -0.57735, 0.57735),
	float3(-0.903007, -0.182696, -0.388844),
	float3(-0.903007, 0.182696, 0.388844),
	float3(0.903007, -0.182696, 0.388844),
	float3(0.903007, 0.182696, -0.388844),
	float3(-0.388844, -0.903007, -0.182696),
	float3(0.388844, -0.903007, 0.182696),
	float3(0.388844, 0.903007, -0.182696),
	float3(-0.388844, 0.903007, 0.182696),
	float3(-0.182696, -0.388844, -0.903007),
	float3(0.182696, 0.388844, -0.903007),
	float3(-0.182696, 0.388844, 0.903007),
	float3(0.182696, -0.388844, 0.903007)
};

float Shadow(float3 worldPosition, float3 normal)
{
	float4 lightSpace = mul(float4(worldPosition, 1.f), ToLight);
	lightSpace /= lightSpace.w;
	
	lightSpace.xy = lightSpace.xy * 0.5f + 0.5f;
	
	if (lightSpace.z > 1.f)
	{
		return 0.f;
	}
	
	float bias = max(0.05f * (1.f - dot(normal, LightDirection)), 0.005f);
	uint3 dimensions;
	ShadowMap.GetDimensions(0, dimensions.x, dimensions.y, dimensions.z);
	float2 texelSize = 1.f / dimensions.xy;
	float shadow = 0.f;
	[unroll]
	for (int x = -2; x < 3; x++)
	{
		[unroll]
		for (int y = -2; y < 3; y++)
		{
			float depth = ShadowMap.Sample(ShadowMapSampler, lightSpace.xy + float2(x, y) * texelSize).r;
			shadow += depth > lightSpace.z + bias ? 1.f : 0.f;
		}
	}
	return shadow / 25.f;
}

float4 ConeTrace(float3 P, float3 N, float3 coneDirection, float coneAperture)
{
	float3 color = 0;
	float alpha = 0;
	
	float dist = VoxelHalfExtent; // offset by cone dir so that first sample of all cones are not the same
	float3 startPos = P + N * VoxelHalfExtent * 2.f * 1.41421f; // sqrt2 is diagonal voxel half-extent

	float maxDistance = VoxelHalfExtent * 2.f * VoxelGridRes;
	while (dist < maxDistance && alpha < 1.f)
	{
		float diameter = max(VoxelHalfExtent, 2 * coneAperture * dist);
		float mip = log2(diameter / VoxelHalfExtent);

		float3 tc = startPos + coneDirection * dist;
		tc = (tc /*- CameraPosition*/) / VoxelHalfExtent;
		tc /= VoxelGridRes;
		tc = tc * float3(0.5f, -0.5f, 0.5f) + 0.5f;

		if (!IsSaturated(tc) || mip >= (float) 9.f)
			break;

		float4 sam = VoxelTexture.SampleLevel(Sampler, tc, mip);

		float a = 1 - alpha;
		color += a * sam.rgb;
		alpha += a * sam.a;

		dist += diameter * (VoxelHalfExtent / 32.f);
	}

	return float4(color, alpha);
}

float4 ConeTraceRadiance(float3 P, float3 N)
{
	float4 radiance = 0;

	for (uint cone = 0; cone < 16; cone++) // quality is between 1 and 16 cones
	{
		float3 coneDirection = normalize(CONES[cone] + N);
		// flip if pointing below
		coneDirection *= dot(coneDirection, N) < 0 ? -1 : 1;

		radiance += ConeTrace(P, N, coneDirection, tan(3.141527f * 0.5f * 0.33f));
	}

	// final radiance is average of all the cones radiances
	radiance /= 16;
	radiance.a = saturate(radiance.a);

	return max(0, radiance);
}

float4 ConeTraceReflection(float3 P, float3 N, float3 V, float roughness)
{
	float aperture = tan(roughness * 3.1415 * 0.5f * 0.1f);
	float3 coneDirection = reflect(-V, N);

	float4 reflection = ConeTrace(P, N, coneDirection, aperture);

	return float4(max(0, reflection.rgb), saturate(reflection.a));
}
