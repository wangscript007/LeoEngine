#ifdef DEBUG
cbuffer LodColor
{
	float4 gColor;
};
#endif

struct PixelIn
{
	float4 PosH : SV_POSITION;
	float2 Tex : TEXCOORD;
};

Texture2D<float4> gAlphaTexture : register(t0);
Texture2DArray gMatTexture :  register(t1);
Texture2D<half3> gNormalMap : register(t2);
SamplerState RepeatLinear:register(s0)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

Texture2D texNormals:register(t3);
SamplerState normalSampler:register(s1);

void CompressUnsignedNormalToNormalsBuffer(inout half3 vNormal)
{
	// renormalize (needed if any blending or interpolation happened before)
	vNormal.rgb = normalize(vNormal.rgb);
	// get unsigned normal for cubemap lookup (note the full float precision is required)
	half3 vNormalUns = abs(vNormal.rgb);
	// get the main axis for cubemap lookup
	half maxNAbs = max(vNormalUns.z, max(vNormalUns.x, vNormalUns.y));
	// get texture coordinates in a collapsed cubemap
	float2 vTexCoord = vNormalUns.z<maxNAbs ? (vNormalUns.y<maxNAbs ? vNormalUns.yz : vNormalUns.xz) : vNormalUns.xy;
	vTexCoord = vTexCoord.x < vTexCoord.y ? vTexCoord.yx : vTexCoord.xy;
	vTexCoord.y /= vTexCoord.x;
	// fit normal into the edge of unit cube
	vNormal.rgb /= maxNAbs;
	// look-up fitting length and scale the normal to get the best fit
	float fFittingScale = texNormals.Sample(normalSampler, vTexCoord).a;
	// scale the normal to get the best fit
	vNormal.rgb *= fFittingScale;
	// squeeze back to unsigned
	vNormal.rgb = vNormal.rgb * .5h + .5h;
}

void main(PixelIn pin,out half4 NormalDepth: SV_TARGET0,
	out float4 DiffuseSpec : SV_TARGET1)
{
	float4 weight = gAlphaTexture.Sample(RepeatLinear, pin.Tex);
	float4 color = gMatTexture.Sample(RepeatLinear, float3(pin.Tex, 0.f));
	float4 c0 = gMatTexture.Sample(RepeatLinear, float3(pin.Tex, 1.f));
	float4 c1 = gMatTexture.Sample(RepeatLinear, float3(pin.Tex, 2.f));
	float4 c2 = gMatTexture.Sample(RepeatLinear, float3(pin.Tex, 3.f));
	float4 c3 = gMatTexture.Sample(RepeatLinear, float3(pin.Tex, 4.f));

	half3 normal = gNormalMap.Sample(RepeatLinear, pin.Tex);
	CompressUnsignedNormalToNormalsBuffer(normal);


	NormalDepth = half4(normal, pin.PosH.z / pin.PosH.w);
	color = lerp(color,c0,weight.r);
	color = lerp(color, c1, weight.g);
	color = lerp(color, c2, weight.b);
	color = lerp(color, c3, weight.a);
#ifdef DEBUG
	DiffuseSpec.xyz = color.xyz*0.75f + gColor.xyz*0.25f;
#else
	DiffuseSpec.xyz = color.xyz;
#endif

	DiffuseSpec.w = 1.f;
}