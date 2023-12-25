#define GROUP_SIZE_X 16
#define GROUP_SIZE_Y 16

#if 0
RWTexture2D <float4> texOutput : register (u0);
RWTexture2D <float4> texInput  : register (u1);

[numthreads (GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void
BltCopy ( uint3 globalIdx : SV_DispatchThreadID )
{
  texOutput [globalIdx.xy] =
   texInput [globalIdx.xy];
}
#else
RWTexture2D <float4> texOutput : register (u0);
RWTexture2D <float4> texInput  : register (u1);

groupshared float4 sharedData [GROUP_SIZE_X * GROUP_SIZE_Y];

[numthreads (GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void
BltCopy ( uint3 globalIdx : SV_DispatchThreadID,
          uint3 localIdx  : SV_GroupThreadID,
          uint3 groupIdx  : SV_GroupID )
{
  // Calculate global and local coordinates
  uint2 globalCoords = globalIdx.xy;
  uint2 localCoords  =  localIdx.xy;

  // Calculate the index within shared memory
  uint sharedIndex =
    localCoords.y * GROUP_SIZE_X + localCoords.x;

  // Load data from the input texture to shared memory
  sharedData [sharedIndex] =
    texInput [globalCoords];

  // Ensure all threads in the group have finished loading before proceeding
  GroupMemoryBarrierWithGroupSync ();

  // Write data from shared memory to the output texture
  texOutput [globalCoords] =
    sharedData [sharedIndex];
}
#endif