cbuffer PerApplication : register( b0 )
{
    matrix projectionMatrix;
}

cbuffer PerFrame : register( b1 )
{
    matrix viewMatrix;
}

cbuffer PerObject : register( b2 )
{
    matrix worldMatrix[512];
    float4 color[512];
}

struct AppData
{
    float3 position : POSITION;
    float3 colora: COLOR;
    uint instanceID : SV_InstanceID;
};

struct VertexShaderOutput
{
    float4 color : COLOR;
    float4 position : SV_POSITION;

};

VertexShaderOutput main(AppData IN)
{
    VertexShaderOutput OUT;

    matrix mvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix[IN.instanceID]));
    OUT.position = mul( mvp, float4( IN.position, 1.0f ) );
    OUT.color = float4(color[IN.instanceID]); ///*IN.color*/, 1.0f);

    return OUT;
}