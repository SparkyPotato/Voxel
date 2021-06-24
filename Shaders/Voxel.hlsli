struct Voxel
{
	uint Direct;
	uint2 Plane;
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

uint2 EncodePlane(float area, float4 plane, float3 voxelCorner)
{
	float normalizedArea = area / (3.4641f * VoxelHalfExtent * VoxelHalfExtent);
	uint intArea = normalizedArea * 255.f;
	uint2 output = uint2(intArea << 24, intArea << 24);
	
	uint3 normalSigns = uint3(plane.x > 0.f, plane.y > 0.f, plane.z > 0.f);
	uint3 intNormal = abs(plane.xyz) * 2047.f;
	
	float t = plane.z - length(voxelCorner);
	float normalizedT = t / (3.4641f * VoxelHalfExtent);
	uint intT = abs(normalizedT) * 2047.f;
	
	output.x |= normalSigns.x | (normalSigns.y << 1) | (intNormal.x << 2) | (intNormal.y << 13);
	output.y |= normalSigns.z | ((t > 0.f) << 1) | (intNormal.z << 2) | (intT << 13);
	
	// x: normal.x > 0 | normal.y > 0 | normal.x | normal.y | area
	// y: normal.z > 0 | t > 0 | normal.z | t | area
	return output;
}

float4 DecodePlane(uint2 mask)
{
	uint3 normalSigns = uint3(mask.x & 1, (mask.x >> 1) & 1, mask.y & 1);
	uint3 intNormal = uint3((mask.x >> 2) & 0x7ff, (mask.x >> 13) & 0x7ff, (mask.y >> 2) & 0x7ff);
	float3 normal = intNormal / 2047.f;
	normal = float3(normalSigns.x ? normal.x : -normal.x, normalSigns.y ? normal.y : -normal.y, normalSigns.z ? normal.z : -normal.z);
	
	uint intT = (mask.y >> 13) & 0x7ff;
	float t = (intT / 2047.f) * 3.4641f * VoxelHalfExtent;
	t = (mask.y >> 1) & 1 ? t : -t;
	
	return float4(normal, t);
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
	
	// float bias = max(0.05f * (1.f - dot(normal, LightDirection)), 0.005f);
	float bias = 0.f;
	uint3 dimensions;
	ShadowMap.GetDimensions(0, dimensions.x, dimensions.y, dimensions.z);
//	float2 texelSize = 1.f / dimensions.xy;
//	float shadow = 0.f;
//	[unroll]
//	for (int x = -1; x < 2; x++)
//	{
//		[unroll]
//		for (int y = -1; y < 2; y++)
//		{
//			float depth = ShadowMap.Sample(ShadowMapSampler, lightSpace.xy + float2(x, y) * texelSize).r;
//			shadow += depth > lightSpace.z + bias ? 1.f : 0.f;
//		}
//	}
//	return shadow / 25.f;
	
	return ShadowMap.Sample(ShadowMapSampler, lightSpace.xy).r > lightSpace.z + bias ? 1.f : 0.f;
}

float4 ConeTrace(float3 P, float3 N, float3 coneDirection, float coneAperture)
{	
	float dist = VoxelHalfExtent; // offset by cone dir so that first sample of all cones are not the same
	float3 startPos = P + N * VoxelHalfExtent * 3.464f; // sqrt2 is diagonal voxel half-extent

	uint _, maxMips;
	VoxelTexture.GetDimensions(0, _, _, _, maxMips);
	
	float maxDistance = VoxelHalfExtent * 2.f * VoxelGridRes;
	while (dist < maxDistance)
	{
		float diameter = max(VoxelHalfExtent, 2 * coneAperture * dist);
		float mip = log2(diameter / VoxelHalfExtent);

		float3 tc = startPos + coneDirection * dist;
		tc = (tc /*- CameraPosition*/) / VoxelHalfExtent;
		tc /= VoxelGridRes;
		tc = tc * float3(0.5f, -0.5f, 0.5f) + 0.5f;

		if (!IsSaturated(tc) || mip >= float(maxMips))
			return 0.f;

		float4 sample = VoxelTexture.SampleLevel(Sampler, tc, mip);
		if (sample.a != 0.f)
		{
			return sample;
		}

		// dist += diameter * (VoxelHalfExtent / 32.f);
		dist += VoxelHalfExtent * 2.f;
	}
	
	return 0.f;
}

float4 ConeTraceRadiance(float3 P, float3 N)
{
	float4 radiance = 0;

	[unroll]
	for (uint cone = 0; cone < 16; cone++) // quality is between 1 and 16 cones
	{
		float3 coneDirection = normalize(CONES[cone] + N);
		coneDirection *= dot(coneDirection, N) < 0 ? -1 : 1; // flip if pointing below

		radiance += ConeTrace(P, N, coneDirection, tan(3.141527f * 0.5f * 0.33f));
	}

	// final radiance is average of all the cones radiances
	radiance /= 16;
	radiance.a = saturate(radiance.a);

	return max(0, radiance);
}

float4 ConeTraceReflection(float3 P, float3 N, float3 V, float roughness)
{
	float aperture = tan(roughness * 3.1415f * 0.05f);
	float3 coneDirection = reflect(-V, N);

	float4 reflection = ConeTrace(P, N, coneDirection, aperture);

	return float4(max(0, reflection.rgb), saturate(reflection.a));
}
