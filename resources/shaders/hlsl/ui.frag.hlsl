/*
 * The GS's MODULATE: on a texture vertex colours run 0..0xff with 0x80 as 1.0, and the product clamps at
 * white. A flat primitive has no MODULATE: its colour goes out as it is, 0x80 is mid grey. As reaches
 * the blender on the 0x80 scale either way: 0x80 is opaque.
 */

Texture2D uTexture : register(t0, space2);
SamplerState uTextureSampler : register(s0, space2);

cbuffer UiFragmentUniforms : register(b0, space3)
{
    float4 uParams; // x = alpha test reference, 0..1 in the same 0x80 scale; y = the colour's scale;
                    // z = 1: the test inverted (only the fragments that fail it: AFAIL's second pass)
};

// The GS compares whole steps of 1 / 0x80: half a step keeps a texel that equals AREF on the passing side,
// whatever the rounding of the products above.
static const float kAlphaTestTolerance = 0.5 / 128.0;

struct PSInput
{
    float4 position : SV_Position;
    float2 vUV : TEXCOORD0;
    float4 vColor : TEXCOORD1;
};

static const float kGsColorScale = 255.0 / 128.0;

float4 main(PSInput input) : SV_Target0
{
    float4 color = uTexture.Sample(uTextureSampler, input.vUV) * input.vColor;
    color.rgb *= uParams.y;
    color.a *= kGsColorScale;
    color = saturate(color);
    const bool passes = color.a + kAlphaTestTolerance >= uParams.x;
    if (passes == (uParams.z > 0.5))
    {
        discard;
    }
    return color;
}
