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
	float4 color;

	input.normal = normalize(input.normal);
	color = ObjTexture.Sample(ObjSamplerState, input.TexCoord);
	//only rgb - we do not want our lighting to affect the alpha channel
	//ambient on the first position
	color.rgb= lights[0].bri* color.rgb;
	//directional light on the second position
	color.rgb+=saturate(dot(lights[1].dir,input.normal)*lights[1].bri*ObjTexture.Sample(ObjSamplerState,input.TexCoord).rgb);
	color.rgb = saturate(color.rgb);
	return color;
}

