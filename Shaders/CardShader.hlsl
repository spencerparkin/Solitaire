// CardShader.hlsl

cbuffer CardConstantsBuffer : register(b0)
{
    float4x4 objToProj;
    float pad[192];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D cardTexture : register(t0);
SamplerState cardSampler : register(s0);

PSInput VSMain(float4 position : POSITION, float2 uv : TEXCOORD)
{
    PSInput output;
    
    output.position = mul(objToProj, position);
    output.uv = uv;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return cardTexture.Sample(cardSampler, input.uv);
}