/*
 * The game's 2D draws, and its 3D ones after their projection (window x, y and Z). Positions come in frame units, the 512x512 frame the GS drew into, which always
 * fills the scene target: how that sits on a window of any shape is DisplayGeometry's business, not this
 * shader's. SDL_GPU's clip space has +Y up, the frame's y runs down.
 */

cbuffer UiVertexUniforms : register(b0, space1)
{
    float4 uFrameToClip; // xy = scale, zw = offset
    float4 uTexelToUv;   // xy = 1 / the texture's size
};

struct VSInput
{
    float3 aPosition : TEXCOORD0; // z: the GS's Z over the Z buffer's largest (0 for the 2D draws)
    float2 aTexel : TEXCOORD1;
    float4 aColor : TEXCOORD2;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 vUV : TEXCOORD0;
    float4 vColor : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    // w 1: the depth is the GS's, interpolated in screen space as the GS does.
    output.position = float4(input.aPosition.xy * uFrameToClip.xy + uFrameToClip.zw, input.aPosition.z, 1.0);
    output.vUV = input.aTexel * uTexelToUv.xy;
    output.vColor = input.aColor;
    return output;
}
