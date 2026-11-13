/*
 * The GS's MODULATE: on a texture vertex colours run 0..0xff with 0x80 as 1.0, and the product clamps at
 * white. A flat primitive has no MODULATE: its colour goes out as it is, 0x80 is mid grey. As reaches
 * the blender on the 0x80 scale either way: 0x80 is opaque.
 *
 * HIGHLIGHT / HIGHLIGHT2 (TEX0's TFX 2 / 3, the lit 3D meshes' specular: the VU1 program writes it into
 * the vertex alpha) add Af to the modulated colour: Cv = Ct Cf + Af, and Av = At + Af (HIGHLIGHT), At
 * (HIGHLIGHT2), Af under TCC 0. uParams.w says which (frame.h TextureFunction).
 */

Texture2D uTexture : register(t0, space2);
SamplerState uTextureSampler : register(s0, space2);

cbuffer UiFragmentUniforms : register(b0, space3)
{
    float4 uParams; // x = alpha test reference, 0..1 in the same 0x80 scale; y = the colour's scale;
                    // z = 1: the test inverted (only the fragments that fail it: AFAIL's second pass);
                    // w = the texture function: 0 MODULATE, 1 HIGHLIGHT, 2 HIGHLIGHT2, 3 either under TCC 0
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

static const int kTfxModulate = 0;
static const int kTfxHighlight = 1;
static const int kTfxHighlight2 = 2;

float4 main(PSInput input) : SV_Target0
{
    const float4 texel = uTexture.Sample(uTextureSampler, input.vUV);
    float4 color = texel * input.vColor;
    color.rgb *= uParams.y;
    color.a *= kGsColorScale;
    const int tfx = (int)(uParams.w + 0.5);
    if (tfx != kTfxModulate)
    {
        // Af on the colour's 0..0xff scale; the texel's alpha is stored doubled (0x80 as 1.0), Af is not.
        color.rgb += input.vColor.a;
        if (tfx == kTfxHighlight)
            color.a = texel.a + input.vColor.a * kGsColorScale;
        else if (tfx == kTfxHighlight2)
            color.a = texel.a;
        else
            color.a = input.vColor.a * kGsColorScale;
    }
    color = saturate(color);
    const bool passes = color.a + kAlphaTestTolerance >= uParams.x;
    if (passes == (uParams.z > 0.5))
    {
        discard;
    }
    return color;
}
