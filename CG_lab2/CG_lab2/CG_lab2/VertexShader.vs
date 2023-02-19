struct VSInput
{
	float3 pos : POSITION;
	float4 color : COLOR;
};

struct VSOutput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

VSOutput vs(VSInput vertex)
{
	VSOutput result;

	result.pos = float4(vertex.pos, 1.0f);
	result.color = vertex.color;

	return result;
}
