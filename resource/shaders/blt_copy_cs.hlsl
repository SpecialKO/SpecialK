RWTexture2D <float4> texOutput : register (u0);
RWTexture2D <float4> texInput  : register (u1);

[numthreads (32, 32, 1)]
void
BltCopy ( uint3 globalIdx : SV_DispatchThreadID )
{
  texOutput [globalIdx.xy] =
   texInput [globalIdx.xy];
}