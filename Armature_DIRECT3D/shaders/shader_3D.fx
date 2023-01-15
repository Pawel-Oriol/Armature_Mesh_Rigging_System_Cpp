struct Light
{
	float3 dir;
	float bri;
};


cbuffer cb_Transforms
{
	float4x4 WVP;
	float4x4 World;
};


cbuffer cb_Lights
{
	Light lights[2];
};

Texture2D ObjTexture;
SamplerState ObjSamplerState;


struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float2 TexCoord: TEXCOORD;
	float3 normal: NORMALS;
};

VS_OUTPUT VS(float4 inPos: POSITION, float2 inTexCoord: TEXCOORD, float3 inNormal: NORMALS)
{
	VS_OUTPUT output;
	output.Pos = mul(inPos, WVP);
	output.TexCoord = inTexCoord;
	output.normal = mul(inNormal,World);

	return output;
}


float4 PS(VS_OUTPUT input) : SV_TARGET
{
	input.normal = normalize(input.normal);
	//ambient na pierwszej pozycji
	float4 color = lights[0].bri*float4(1,1,1,1);
	//swiatlo kierunkowe na drugiej pozycji
	color+=saturate(dot(lights[1].dir,input.normal)*lights[1].bri*float4(1,1,1,1));
	color = saturate(color);
	return color;
}

