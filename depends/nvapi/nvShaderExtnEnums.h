////////////////////////////////////////////////////////////////////////////////
////////////////////////// NVIDIA SHADER EXTENSIONS ////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// This file can be included both from HLSL shader code as well as C++ code.
// The app should call NvAPI_D3D_IsNvShaderExtnOpCodeSupported() to 
// check for support for every nv shader extension opcode it plans to use



//----------------------------------------------------------------------------//
//---------------------------- NV Shader Extn Version  -----------------------//
//----------------------------------------------------------------------------//
#define NV_SHADER_EXTN_VERSION                              1

//----------------------------------------------------------------------------//
//---------------------------- Misc constants --------------------------------//
//----------------------------------------------------------------------------//
#define NV_WARP_SIZE                                       32


//----------------------------------------------------------------------------//
//---------------------------- opCode constants ------------------------------//
//----------------------------------------------------------------------------//


#define NV_EXTN_OP_SHFL                                     1
#define NV_EXTN_OP_SHFL_UP                                  2
#define NV_EXTN_OP_SHFL_DOWN                                3
#define NV_EXTN_OP_SHFL_XOR                                 4

#define NV_EXTN_OP_VOTE_ALL                                 5
#define NV_EXTN_OP_VOTE_ANY                                 6
#define NV_EXTN_OP_VOTE_BALLOT                              7

#define NV_EXTN_OP_GET_LANE_ID                              8
#define NV_EXTN_OP_FP16_ATOMIC                             12
#define NV_EXTN_OP_FP32_ATOMIC                             13
