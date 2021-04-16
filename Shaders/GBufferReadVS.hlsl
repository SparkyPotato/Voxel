struct VSOut
{
	float2 UV : UV;
	float4 Position : SV_POSITION;
};

VSOut main(uint id : SV_VertexID)
{
	VSOut output;

	output.UV = float2((id << 1) & 2, id & 2);
	output.Position = float4(output.UV * float2(2.f, -2.f) + float2(-1.f, 1.f), 0.f, 1.f);

	return output;
}
