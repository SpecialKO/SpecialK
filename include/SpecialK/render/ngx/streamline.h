/*
* Copyright (c) 2022-2023 NVIDIA CORPORATION. All rights reserved
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#pragma once

#define SL_VERSION_MAJOR 2
#define SL_VERSION_MINOR 4
#define SL_VERSION_PATCH 15

#define SL_ENUM_OPERATORS_64(T)                                                         \
inline bool operator&(T a, T b)                                                         \
{                                                                                       \
    return ((uint64_t)a & (uint64_t)b) != 0;                                            \
}                                                                                       \
                                                                                        \
inline T& operator&=(T& a, T b)                                                         \
{                                                                                       \
    a = (T)((uint64_t)a & (uint64_t)b);                                                 \
    return a;                                                                           \
}                                                                                       \
                                                                                        \
inline T operator|(T a, T b)                                                            \
{                                                                                       \
    return (T)((uint64_t)a | (uint64_t)b);                                              \
}                                                                                       \
                                                                                        \
inline T& operator |= (T& lhs, T rhs)                                                   \
{                                                                                       \
    lhs = (T)((uint64_t)lhs | (uint64_t)rhs);                                           \
    return lhs;                                                                         \
}                                                                                       \
                                                                                        \
inline T operator~(T a)                                                                 \
{                                                                                       \
    return (T)~((uint64_t)a);                                                           \
}

#define SL_ENUM_OPERATORS_32(T)                                                         \
inline bool operator&(T a, T b)                                                         \
{                                                                                       \
    return ((uint32_t)a & (uint32_t)b) != 0;                                            \
}                                                                                       \
                                                                                        \
inline T& operator&=(T& a, T b)                                                         \
{                                                                                       \
    a = (T)((uint32_t)a & (uint32_t)b);                                                 \
    return a;                                                                           \
}                                                                                       \
                                                                                        \
inline T operator|(T a, T b)                                                            \
{                                                                                       \
    return (T)((uint32_t)a | (uint32_t)b);                                              \
}                                                                                       \
                                                                                        \
inline T& operator |= (T& lhs, T rhs)                                                   \
{                                                                                       \
    lhs = (T)((uint32_t)lhs | (uint32_t)rhs);                                           \
    return lhs;                                                                         \
}                                                                                       \
                                                                                        \
inline T operator~(T a)                                                                 \
{                                                                                       \
    return (T)~((uint32_t)a);                                                           \
}


namespace sl
{
constexpr uint64_t kSDKVersionMagic = 0xfedc;
constexpr uint64_t kSDKVersion = (uint64_t(SL_VERSION_MAJOR) << 48) | (uint64_t(SL_VERSION_MINOR) << 32) | (uint64_t(SL_VERSION_PATCH) << 16) | kSDKVersionMagic;

struct Version
{
    Version() : major(0), minor(0), build(0) {};
    Version(uint32_t v1, uint32_t v2, uint32_t v3) : major(v1), minor(v2), build(v3) {};

    inline operator bool() const { return major != 0 || minor != 0 || build != 0; }

    inline std::string toStr() const
    {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(build);
    }
    inline std::wstring toWStr() const
    {
        return std::to_wstring(major) + L"." + std::to_wstring(minor) + L"." + std::to_wstring(build);
    }
    inline std::wstring toWStrOTAId() const
    {
        return std::to_wstring((major << 16) | (minor << 8) | build);
    }
    inline bool operator==(const Version& rhs) const
    {
        return major == rhs.major && minor == rhs.minor && build == rhs.build;
    }
    inline bool operator>(const Version& rhs) const
    {
        if (major < rhs.major) return false;
        else if (major > rhs.major) return true;
        // major version the same
        if (minor < rhs.minor) return false;
        else if (minor > rhs.minor) return true;
        // minor version the same
        if (build < rhs.build) return false;
        else if (build > rhs.build) return true;
        // build version the same
        return false;
    };
    inline bool operator>=(const Version& rhs) const
    {
        return operator>(rhs) || operator==(rhs);
    };
    inline bool operator<(const Version& rhs) const
    {
        if (major > rhs.major) return false;
        else if (major < rhs.major) return true;
        // major version the same
        if (minor > rhs.minor) return false;
        else if (minor < rhs.minor) return true;
        // minor version the same
        if (build > rhs.build) return false;
        else if (build < rhs.build) return true;
        // build version the same
        return false;
    };
    inline bool operator<=(const Version& rhs) const
    {
        return operator<(rhs) || operator==(rhs);
    };

    uint32_t major;
    uint32_t minor;
    uint32_t build;
};

enum class Result
{
  eOk,
  eErrorIO,
  eErrorDriverOutOfDate,
  eErrorOSOutOfDate,
  eErrorOSDisabledHWS,
  eErrorDeviceNotCreated,
  eErrorNoSupportedAdapterFound,
  eErrorAdapterNotSupported,
  eErrorNoPlugins,
  eErrorVulkanAPI,
  eErrorDXGIAPI,
  eErrorD3DAPI,
  eErrorNRDAPI,
  eErrorNVAPI,
  eErrorReflexAPI,
  eErrorNGXFailed,
  eErrorJSONParsing,
  eErrorMissingProxy,
  eErrorMissingResourceState,
  eErrorInvalidIntegration,
  eErrorMissingInputParameter,
  eErrorNotInitialized,
  eErrorComputeFailed,
  eErrorInitNotCalled,
  eErrorExceptionHandler,
  eErrorInvalidParameter,
  eErrorMissingConstants,
  eErrorDuplicatedConstants,
  eErrorMissingOrInvalidAPI,
  eErrorCommonConstantsMissing,
  eErrorUnsupportedInterface,
  eErrorFeatureMissing,
  eErrorFeatureNotSupported,
  eErrorFeatureMissingHooks,
  eErrorFeatureFailedToLoad,
  eErrorFeatureWrongPriority,
  eErrorFeatureMissingDependency,
  eErrorFeatureManagerInvalidState,
  eErrorInvalidState,
  eWarnOutOfVRAM
};

//! GUID
struct StructType
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];

    inline bool operator==(const StructType& rhs) const { return memcmp(this, &rhs, sizeof(*this)) == 0; }
    inline bool operator!=(const StructType& rhs) const { return memcmp(this, &rhs, sizeof(*this)) != 0; }
};

//! SL is using typed and versioned structures which can be chained or not.
//! 
//! --- OPTION 1 ---
//! 
//! New members must be added at the end and version needs to be increased:
//! 
//! SL_STRUCT(S1, GUID1, kStructVersion1)
//! {
//!     A
//!     B
//!     C
//! }
//! 
//! SL_STRUCT(S1, GUID1, kStructVersion2) // Note that version is bumped
//! {
//!     // V1
//!     A
//!     B
//!     C
//! 
//!     //! V2 - new members always go at the end!
//!     D
//!     E
//! }
//! 
//! Here is one example on how to check for version and handle backwards compatibility:
//! 
//! void func(const S1* input)
//! {
//!     // Access A, B, C as needed
//!     ...
//!     if (input->structVersion >= kStructVersion2)
//!     {
//!         // Safe to access D, E
//!     }
//! }


//! --- OPTION 2 ---
//! 
//! New members are optional and added to a new struct which is then chained as needed:
//! 
//! SL_STRUCT(S1, GUID1, kStructVersion1)
//! {
//!     A
//!     B
//!     C
//! }
//! 
//! SL_STRUCT(S2, GUID2, kStructVersion1) // Note that this is a different struct with new GUID
//! {
//!     D
//!     E
//! }
//! 
//! S1 s1;
//! S2 s2
//! s1.next = &s2; // optional parameters in S2

//! IMPORTANT: New members in the structure always go at the end!
//!
constexpr uint32_t kStructVersion1 = 1;
constexpr uint32_t kStructVersion2 = 2;
constexpr uint32_t kStructVersion3 = 3;

struct BaseStructure
{
    BaseStructure() = delete;
    BaseStructure(StructType t, uint32_t v) : structType(t), structVersion(v) {};
    BaseStructure* next{};
    StructType structType{};
    size_t structVersion;
};

#define SL_STRUCT(name, guid, version)                                      \
struct name : public BaseStructure                                          \
{                                                                           \
    name() : BaseStructure(guid, version){}                                 \
    constexpr static StructType s_structType = guid;                        \

#define SL_STRUCT_PROTECTED(name, guid, version)                            \
struct name : public BaseStructure                                          \
{                                                                           \
protected:                                                                  \
    name() : BaseStructure(guid, version){}                                 \
public:                                                                     \
    constexpr static StructType s_structType = guid;                        \




//! For cases when value has to be provided and we don't have good default
constexpr float INVALID_FLOAT = 3.40282346638528859811704183484516925440e38f;
constexpr uint32_t INVALID_UINT = 0xffffffff;

struct uint3
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

struct float2
{
    float2() : x(INVALID_FLOAT), y(INVALID_FLOAT) {}
    float2(float _x, float _y) : x(_x), y(_y) {}
    float x, y;
};

struct float3
{
    float3() : x(INVALID_FLOAT), y(INVALID_FLOAT), z(INVALID_FLOAT) {}
    float3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    float x, y, z;
};

struct float4
{
    float4() : x(INVALID_FLOAT), y(INVALID_FLOAT), z(INVALID_FLOAT), w(INVALID_FLOAT) {}
    float4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    float x, y, z, w;
};

struct float4x4
{
    //! All access points take row index as a parameter
    inline float4& operator[](uint32_t i) { return row[i]; }
    inline const float4& operator[](uint32_t i) const { return row[i]; }
    inline void setRow(uint32_t i, const float4& v) { row[i] = v; }
    inline const float4& getRow(uint32_t i) { return row[i]; }

    //! Row major matrix
    float4 row[4];
};

struct Extent
{
    uint32_t top{};
    uint32_t left{};
    uint32_t width{};
    uint32_t height{};

    inline operator bool() const { return width != 0 && height != 0; }
    inline bool operator==(const Extent& rhs) const 
    { 
        return top == rhs.top && left == rhs.left &&
        width == rhs.width && height == rhs.height;
    }
    inline bool operator!=(const Extent& rhs) const
    {
        return !operator==(rhs);
    }
    inline bool isSameRes(const Extent& rhs) const
    {
        return width == rhs.width && height == rhs.height;
    }

#if defined(_WINDEF_)
    // Cast helper for sl::Extent->RECT when windef.h has been included
    inline operator RECT() const { return RECT { (LONG)left, (LONG)top, (LONG)(left + width), (LONG)(top + height) }; }
#endif
};

//! For cases when value has to be provided and we don't have good default
enum Boolean : char
{
    eFalse,
    eTrue,
    eInvalid
};

//! Common constants, all parameters must be provided unless they are marked as optional
//! 
//! {DCD35AD7-4E4A-4BAD-A90C-E0C49EB23AFE}
SL_STRUCT(Constants, StructType({ 0xdcd35ad7, 0x4e4a, 0x4bad, { 0xa9, 0xc, 0xe0, 0xc4, 0x9e, 0xb2, 0x3a, 0xfe } }), kStructVersion2)
    //! IMPORTANT: All matrices are row major (see float4x4 definition) and
    //! must NOT contain temporal AA jitter offset (if any). Any jitter offset
    //! should be provided as the additional parameter Constants::jitterOffset (see below)
            
    //! Specifies matrix transformation from the camera view to the clip space.
    float4x4 cameraViewToClip;
    //! Specifies matrix transformation from the clip space to the camera view space.
    float4x4 clipToCameraView;
    //! Optional - Specifies matrix transformation describing lens distortion in clip space.
    float4x4 clipToLensClip;
    //! Specifies matrix transformation from the current clip to the previous clip space.
    //! clipToPrevClip = clipToView * viewToViewPrev * viewToClipPrev
    //! Sample code can be found in sl_matrix_helpers.h
    float4x4 clipToPrevClip;
    //! Specifies matrix transformation from the previous clip to the current clip space.
    //! prevClipToClip = clipToPrevClip.inverse()
    float4x4 prevClipToClip;
        
    //! Specifies pixel space jitter offset
    float2 jitterOffset;
    //! Specifies scale factors used to normalize motion vectors (so the values are in [-1,1] range)
    float2 mvecScale;
    //! Optional - Specifies camera pinhole offset if used.
    float2 cameraPinholeOffset;
    //! Specifies camera position in world space.
    float3 cameraPos;
    //! Specifies camera up vector in world space.
    float3 cameraUp;
    //! Specifies camera right vector in world space.
    float3 cameraRight;
    //! Specifies camera forward vector in world space.
    float3 cameraFwd;
        
    //! Specifies camera near view plane distance.
    float cameraNear = INVALID_FLOAT;
    //! Specifies camera far view plane distance.
    float cameraFar = INVALID_FLOAT;
    //! Specifies camera field of view in radians.
    float cameraFOV = INVALID_FLOAT;
    //! Specifies camera aspect ratio defined as view space width divided by height.
    float cameraAspectRatio = INVALID_FLOAT;
    //! Specifies which value represents an invalid (un-initialized) value in the motion vectors buffer
    //! NOTE: This is only required if `cameraMotionIncluded` is set to false and SL needs to compute it.
    float motionVectorsInvalidValue = INVALID_FLOAT;

    //! Specifies if depth values are inverted (value closer to the camera is higher) or not.
    Boolean depthInverted = Boolean::eInvalid;
    //! Specifies if camera motion is included in the MVec buffer.
    Boolean cameraMotionIncluded = Boolean::eInvalid;
    //! Specifies if motion vectors are 3D or not.
    Boolean motionVectors3D = Boolean::eInvalid;
    //! Specifies if previous frame has no connection to the current one (i.e. motion vectors are invalid)
    Boolean reset = Boolean::eInvalid;
    //! Specifies if orthographic projection is used or not.
    Boolean orthographicProjection = Boolean::eFalse;
    //! Specifies if motion vectors are already dilated or not.
    Boolean motionVectorsDilated = Boolean::eFalse;
    //! Specifies if motion vectors are jittered or not.
    Boolean motionVectorsJittered = Boolean::eFalse;

    //! Version 2 members:
    //! 
    //! Optional heuristic that specifies the minimum depth difference between two objects in screen-space.
    //! The units of the value are in linear depth units.
    //! Linear depth is computed as:
    //!     if depthInverted is false:  `lin_depth = 1 / (1 - depth)` 
    //!     if depthInverted is true:   `lin_depth = 1 / depth`
    //! 
    //! Although unlikely to need to be modified, smaller thresholds are useful when depth units are
    //! unusually compressed into a small dynamic range near 1.
    //! 
    //! If not specified, the default value is 40.0f.
    float minRelativeLinearDepthObjectSeparation = 40.0f;

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};



using CommandBuffer = void;
using Device = void;

//! Buffer types used for tagging
//! 
//! IMPORTANT: Each tag must use the unique id
//! 
using BufferType = uint32_t;

//! Depth buffer - IMPORTANT - Must be suitable to use with clipToPrevClip transformation (see Constants below)
constexpr BufferType kBufferTypeDepth = 0;
//! Object and optional camera motion vectors (see Constants below)
constexpr BufferType kBufferTypeMotionVectors = 1;
//! Color buffer with all post-processing effects applied but without any UI/HUD elements
constexpr BufferType kBufferTypeHUDLessColor = 2;
//! Color buffer containing jittered input data for the image scaling pass
constexpr BufferType kBufferTypeScalingInputColor = 3;
//! Color buffer containing results from the image scaling pass
constexpr BufferType kBufferTypeScalingOutputColor = 4;
//! Normals
constexpr BufferType kBufferTypeNormals = 5;
//! Roughness
constexpr BufferType kBufferTypeRoughness = 6;
//! Albedo
constexpr BufferType kBufferTypeAlbedo = 7;
//! Specular Albedo
constexpr BufferType kBufferTypeSpecularAlbedo = 8;
//! Indirect Albedo
constexpr BufferType kBufferTypeIndirectAlbedo = 9;
//! Specular Motion Vectors
constexpr BufferType kBufferTypeSpecularMotionVectors = 10;
//! Disocclusion Mask
constexpr BufferType kBufferTypeDisocclusionMask = 11;
//! Emissive
constexpr BufferType kBufferTypeEmissive = 12;
//! Exposure
constexpr BufferType kBufferTypeExposure = 13;
//! Buffer with normal and roughness in alpha channel
constexpr BufferType kBufferTypeNormalRoughness = 14;
//! Diffuse and camera ray length
constexpr BufferType kBufferTypeDiffuseHitNoisy = 15;
//! Diffuse denoised
constexpr BufferType kBufferTypeDiffuseHitDenoised = 16;
//! Specular and reflected ray length
constexpr BufferType kBufferTypeSpecularHitNoisy = 17;
//! Specular denoised
constexpr BufferType kBufferTypeSpecularHitDenoised = 18;
//! Shadow noisy
constexpr BufferType kBufferTypeShadowNoisy = 19;
//! Shadow denoised
constexpr BufferType kBufferTypeShadowDenoised = 20;
//! AO noisy
constexpr BufferType kBufferTypeAmbientOcclusionNoisy = 21;
//! AO denoised
constexpr BufferType kBufferTypeAmbientOcclusionDenoised = 22;
//! Optional - UI/HUD color and alpha
//! IMPORTANT: Please make sure that alpha channel has enough precision (for example do NOT use formats like R10G10B10A2)
constexpr BufferType kBufferTypeUIColorAndAlpha = 23;
//! Optional - Shadow pixels hint (set to 1 if a pixel belongs to the shadow area, 0 otherwise)
constexpr BufferType kBufferTypeShadowHint = 24;
//! Optional - Reflection pixels hint (set to 1 if a pixel belongs to the reflection area, 0 otherwise)
constexpr BufferType kBufferTypeReflectionHint = 25;
//! Optional - Particle pixels hint (set to 1 if a pixel represents a particle, 0 otherwise)
constexpr BufferType kBufferTypeParticleHint = 26;
//! Optional - Transparency pixels hint (set to 1 if a pixel belongs to the transparent area, 0 otherwise)
constexpr BufferType kBufferTypeTransparencyHint = 27;
//! Optional - Animated texture pixels hint (set to 1 if a pixel belongs to the animated texture area, 0 otherwise)
constexpr BufferType kBufferTypeAnimatedTextureHint = 28;
//! Optional - Bias for current color vs history hint - lerp(history, current, bias) (set to 1 to completely reject history)
constexpr BufferType kBufferTypeBiasCurrentColorHint = 29;
//! Optional - Ray-tracing distance (camera ray length)
constexpr BufferType kBufferTypeRaytracingDistance = 30;
//! Optional - Motion vectors for reflections
constexpr BufferType kBufferTypeReflectionMotionVectors = 31;
//! Optional - Position, in same space as eNormals
constexpr BufferType kBufferTypePosition = 32;
//! Optional - Indicates (via non-zero value) which pixels have motion/depth values that do not match the final color content at that pixel (e.g. overlaid, opaque Picture-in-Picture)
constexpr BufferType kBufferTypeInvalidDepthMotionHint = 33;
//! Alpha
constexpr BufferType kBufferTypeAlpha = 34;
//! Color buffer containing only opaque geometry
constexpr BufferType kBufferTypeOpaqueColor = 35;
//! Optional - Reduce reliance on history instead using current frame hint (0 if a pixel is not at all reactive and default composition should be used, 1 if fully reactive)
constexpr BufferType kBufferTypeReactiveMaskHint = 36;
//! Optional - Pixel lock adjustment hint (set to 1 if pixel lock should be completely removed, 0 otherwise)
constexpr BufferType kBufferTypeTransparencyAndCompositionMaskHint = 37;
//! Optional - Albedo of the reflection ray hit point. For multibounce reflections, this should be the albedo of the first non-specular bounce.
constexpr BufferType kBufferTypeReflectedAlbedo = 38;
//! Optional - Color buffer before particles are drawn.
constexpr BufferType kBufferTypeColorBeforeParticles = 39;
//! Optional - Color buffer before transparent objects are drawn.
constexpr BufferType kBufferTypeColorBeforeTransparency = 40;
//! Optional - Color buffer before fog is drawn.
constexpr BufferType kBufferTypeColorBeforeFog = 41;
//! Optional - Buffer containing the hit distance of a specular ray.
constexpr BufferType kBufferTypeSpecularHitDistance = 42;
//! Optional - Buffer that contains 3 components of a specular ray direction, and 1 component of specular hit distance.
constexpr BufferType kBufferTypeSpecularRayDirectionHitDistance = 43;
//! Optional - Buffer containing normalized direction of a specular ray.
constexpr BufferType kBufferTypeSpecularRayDirection = 44;
// !Optional - Buffer containing the hit distance of a diffuse ray.
constexpr BufferType kBufferTypeDiffuseHitDistance = 45;
//! Optional - Buffer that contains 3 components of a diffuse ray direction, and 1 component of diffuse hit distance.
constexpr BufferType kBufferTypeDiffuseRayDirectionHitDistance = 46;
//! Optional - Buffer containing normalized direction of a diffuse ray.
constexpr BufferType kBufferTypeDiffuseRayDirection = 47;
//! Optional - Buffer containing display resolution depth.
constexpr BufferType kBufferTypeHiResDepth = 48;
//! Required either this or kBufferTypeDepth - Buffer containing linear depth.
constexpr BufferType kBufferTypeLinearDepth = 49;
//! Optional - Bidirectional distortion field. 4 channels in normalized [0,1] pixel space. RG = distorted pixel to undistorted pixel displacement. BA = undistorted pixel to distorted pixel displacement.
constexpr BufferType kBufferTypeBidirectionalDistortionField = 50;
//!Optional - Buffer containing particles or other similar transparent effects rendered into it instead of passing it as part of the input color
constexpr BufferType kBufferTypeTransparencyLayer = 51;
//!Optional - Butffer to be used in addition to TransparencyLayer which allows 3-channels of Opacity versus 1-channel. 
//            In this case, TransparencyLayer represents Color (RcGcBc), TransparencyLayerOpacity represents alpha (RaGaBa)'
constexpr BufferType kBufferTypeTransparencyLayerOpacity = 52;
//! Optional - Swapchain buffer to be presented
constexpr BufferType kBufferTypeBackbuffer = 53;

//! Features supported with this SDK
//! 
//! IMPORTANT: Each feature must use a unique id
//! 
using Feature = uint32_t;

//! Deep Learning Super Sampling
constexpr Feature kFeatureDLSS = 0;

//! Real-Time Denoiser
constexpr Feature kFeatureNRD = 1;

//! NVIDIA Image Scaling
constexpr Feature kFeatureNIS = 2;

//! Reflex
constexpr Feature kFeatureReflex = 3;

//! PC Latency
constexpr Feature kFeaturePCL = 4;

//! DeepDVC
constexpr Feature kFeatureDeepDVC = 5;


//! DLSS Frame Generation
constexpr Feature kFeatureDLSS_G = 1000;

//! DLSS Ray Reconstruction
constexpr Feature kFeatureDLSS_RR = 1001;

constexpr Feature kFeatureNvPerf = 1002;

constexpr Feature kFeatureDirectSR = 1003;

// ImGUI 
constexpr Feature kFeatureImGUI = 9999;

//! Common feature, NOT intended to be used directly
constexpr Feature kFeatureCommon = UINT_MAX;

//! Different levels for logging
enum class LogLevel : uint32_t
{
    //! No logging
    eOff,
    //! Default logging
    eDefault,
    //! Verbose logging
    eVerbose,
    //! Total count
    eCount
};

//! Resource types
enum class ResourceType : char
{
    eTex2d,
    eBuffer,
    eCommandQueue,
    eCommandBuffer,
    eCommandPool,
    eFence,
    eSwapchain,
    eHostFence,
    eCount
};

//! Resource allocate information
//!
SL_STRUCT(ResourceAllocationDesc, StructType({ 0xbb57e5, 0x49a2, 0x4c23, { 0xa5, 0x19, 0xab, 0x92, 0x86, 0xe7, 0x40, 0x14 } }), kStructVersion1)
    ResourceAllocationDesc(ResourceType _type, void* _desc, uint32_t _state, void* _heap) : BaseStructure(ResourceAllocationDesc::s_structType, kStructVersion1), type(_type),desc(_desc),state(_state),heap(_heap){};
    //! Indicates the type of resource
    ResourceType type = ResourceType::eTex2d;
    //! D3D12_RESOURCE_DESC/VkImageCreateInfo/VkBufferCreateInfo
    void* desc{};
    //! Initial state as D3D12_RESOURCE_STATES or VkMemoryPropertyFlags
    uint32_t state = 0;
    //! CD3DX12_HEAP_PROPERTIES or nullptr
    void* heap{};

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};


//! Subresource range information, for Vulkan resources
//! 
//! {8D4C316C-D402-4524-89A7-14E79E638E3A}
SL_STRUCT(SubresourceRange, StructType({ 0x8d4c316c, 0xd402, 0x4524, { 0x89, 0xa7, 0x14, 0xe7, 0x9e, 0x63, 0x8e, 0x3a } }), kStructVersion1)
    //! Vulkan subresource aspectMask
    uint32_t aspectMask;
    //! Vulkan subresource baseMipLevel
    uint32_t baseMipLevel;
    //! Vulkan subresource levelCount
    uint32_t levelCount;
    //! Vulkan subresource baseArrayLayer
    uint32_t baseArrayLayer;
    //! Vulkan subresource layerCount
    uint32_t layerCount;
};

//! Native resource
//! 
//! {3A9D70CF-2418-4B72-8391-13F8721C7261}
SL_STRUCT(Resource, StructType({ 0x3a9d70cf, 0x2418, 0x4b72, { 0x83, 0x91, 0x13, 0xf8, 0x72, 0x1c, 0x72, 0x61 } }), kStructVersion1)
    //! Constructors
    //! 
    //! Resource type, native pointer are MANDATORY always
    //! Resource state is MANDATORY unless using D3D11
    //! Resource view, description etc. are MANDATORY only when using Vulkan
    //! 
    Resource(ResourceType _type, void* _native, void* _mem, void* _view, uint32_t _state = UINT_MAX) : BaseStructure(Resource::s_structType, kStructVersion1), type(_type), native(_native), memory(_mem), view(_view), state(_state){};
    Resource(ResourceType _type, void* _native, uint32_t _state = UINT_MAX) : BaseStructure(Resource::s_structType, kStructVersion1), type(_type), native(_native), state(_state) {};

    //! Conversion helpers for D3D
    inline operator ID3D12Resource* () { return reinterpret_cast<ID3D12Resource*>(native); }
    inline operator ID3D11Resource* () { return reinterpret_cast<ID3D11Resource*>(native); }
    inline operator ID3D11Buffer* () { return reinterpret_cast<ID3D11Buffer*>(native); }
    inline operator ID3D11Texture2D* () { return reinterpret_cast<ID3D11Texture2D*>(native); }

    //! Indicates the type of resource
    ResourceType type = ResourceType::eTex2d;
    //! ID3D11Resource/ID3D12Resource/VkBuffer/VkImage
    void* native{};
    //! vkDeviceMemory or nullptr
    void* memory{};
    //! VkImageView/VkBufferView or nullptr
    void* view{};
    //! State as D3D12_RESOURCE_STATES or VkImageLayout
    //! 
    //! IMPORTANT: State is MANDATORY and needs to be correct when tagged resources are actually used.
    //! 
    uint32_t state = UINT_MAX;
    //! Width in pixels
    uint32_t width{};
    //! Height in pixels
    uint32_t height{};
    //! Native format
    uint32_t nativeFormat{};
    //! Number of mip-map levels
    uint32_t mipLevels{};
    //! Number of arrays
    uint32_t arrayLayers{};
    //! Virtual address on GPU (if applicable)
    uint64_t gpuVirtualAddress{};
    //! VkImageCreateFlags
    uint32_t flags;
    //! VkImageUsageFlags
    uint32_t usage{};
    //! Reserved for internal use
    uint32_t reserved{};

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};

//! Specifies life-cycle for the tagged resource
//! 
//! IMPORTANT: Use 'eOnlyValidNow' and 'eValidUntilEvaluate' ONLY when really needed since it can result in wasting VRAM if SL ends up making unnecessary copies.
//! 
//! If integrating features, like for example DLSS-G, which require tags to be 'eValidUntilPresent' please try to tag everything as 'eValidUntilPresent' first
//! and only make modifications if upon visual inspection you notice that tags are corrupted when used during the Present frame call.
enum ResourceLifecycle
{
    //! Resource can change, get destroyed or reused for other purposes after it is provided to SL
    eOnlyValidNow,
    //! Resource does NOT change, gets destroyed or reused for other purposes from the moment it is provided to SL until the frame is presented
    eValidUntilPresent,
    //! Resource does NOT change, gets destroyed or reused for other purposes from the moment it is provided to SL until after the slEvaluateFeature call has returned.
    eValidUntilEvaluate
};

//! Tagged resource
//! 
//! {4C6A5AAD-B445-496C-87FF-1AF3845BE653}
//! Extensions as part of the `next` ptr:
//!     PrecisionInfo
SL_STRUCT(ResourceTag, StructType({ 0x4c6a5aad, 0xb445, 0x496c, { 0x87, 0xff, 0x1a, 0xf3, 0x84, 0x5b, 0xe6, 0x53 } }), kStructVersion1)
    ResourceTag(Resource* r, BufferType t, ResourceLifecycle l, const Extent* e = nullptr)
        : BaseStructure(ResourceTag::s_structType, kStructVersion1), resource(r), type(t), lifecycle(l)
    {
        if (e) extent = *e;
    };

    //! Resource description
    Resource* resource{};
    //! Type of the tagged buffer
    BufferType type{};
    //! The life-cycle for the tag, if resource is volatile a valid command buffer must be specified
    ResourceLifecycle lifecycle{};
    //! The area of the tagged resource to use (if using the entire resource leave as null)
    Extent extent{};

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};

// 
//! Precision info, optional extension for ResourceTag.
//! 
//! {98F6E9BA-8D16-4831-A802-4D3B52FF26BF}
//! Extensions as part of the `next` ptr:
//!     ResourceTag
SL_STRUCT(PrecisionInfo, StructType({ 0x98f6e9ba, 0x8d16, 0x4831, { 0xa8, 0x2, 0x4d, 0x3b, 0x52, 0xff, 0x26, 0xbf } }), kStructVersion1)
    // Formula used to convert the low-precision data to high-precision
    enum PrecisionFormula : uint32_t
    {
        eNoTransform = 0,           // hi = lo, essentially no conversion is done
        eLinearTransform,           // hi = lo * scale + bias
    };

    PrecisionInfo(PrecisionInfo::PrecisionFormula formula, float bias, float scale)
        : BaseStructure(PrecisionInfo::structType, kStructVersion1), conversionFormula(formula), bias(bias), scale(scale) {};
    
    static std::string getPrecisionFormulaAsStr(PrecisionFormula formula)
    {
        switch (formula)
        {
        case eNoTransform:
            return "eNoTransform";
        case eLinearTransform:
            return "eLinearTransform";
        default:
            assert("Invalid PrecisionFormula" && false);
            return "Unknown";
        }
    };

    PrecisionFormula conversionFormula{ eNoTransform };
    float bias{ 0.0f };
    float scale{ 1.0f };

    inline operator bool() const { return conversionFormula != eNoTransform; }
    inline bool operator==(const PrecisionInfo& rhs) const
    {
        return conversionFormula == rhs.conversionFormula && bias == rhs.bias && scale == rhs.scale;
    }
    inline bool operator!=(const PrecisionInfo& rhs) const
    {
        return !operator==(rhs);
    }
};

//! Resource allocation/deallocation callbacks
//!
//! Use these callbacks to gain full control over 
//! resource life cycle and memory allocation tracking.
//!
//! @param device - Device to be used (vkDevice or ID3D11Device or ID3D12Device)
//!
//! IMPORTANT: Textures must have the pixel shader resource
//! and the unordered access view flags set
using PFun_ResourceAllocateCallback = Resource(const ResourceAllocationDesc* desc, void* device);
using PFun_ResourceReleaseCallback = void(Resource* resource, void* device);

//! Log type
enum class LogType : uint32_t
{
    //! Controlled by LogLevel, SL can show more information in eLogLevelVerbose mode
    eInfo,
    //! Always shown regardless of LogLevel
    eWarn,
    eError,
    //! Total count
    eCount
};

//! Logging callback
//!
//! Use these callbacks to track messages posted in the log.
//! If any of the SL methods returns false use eLogTypeError
//! type to track down what went wrong and why.
using PFun_LogMessageCallback = void(LogType type, const char* msg);

//! Optional flags
enum class PreferenceFlags : uint64_t
{
    //! Set by default - Disables command list state tracking - Host application is responsible for restoring CL state correctly after each 'slEvaluateFeature' call
    eDisableCLStateTracking = 1 << 0,
    //! Optional - Disables debug text on screen in development builds
    eDisableDebugText = 1 << 1,
    //! Optional - IMPORTANT: Only to be used in the advanced integration mode, see the 'manual hooking' programming guide for more details
    eUseManualHooking = 1 << 2,
    //! Optional - Enables downloading of Over The Air (OTA) updates for SL and NGX
    //! This will invoke the OTA updater to look for new updates. A separate
    //! flag below is used to control whether or not OTA-downloaded SL Plugins are
    //! loaded.
    eAllowOTA = 1 << 3,
    //! Do not check OS version when deciding if feature is supported or not
    //! 
    //! IMPORTANT: ONLY SET THIS FLAG IF YOU KNOW WHAT YOU ARE DOING. 
    //! 
    //! VARIOUS WIN APIs INCLUDING BUT NOT LIMITED TO `IsWindowsXXX`, `GetVersionX`, `rtlGetVersion` ARE KNOWN FOR RETURNING INCORRECT RESULTS.
    eBypassOSVersionCheck = 1 << 4,
    //! Optional - If specified SL will create DXGI factory proxy rather than modifying the v-table for the base interface.
    //! 
    //! This can help with 3rd party overlays which are NOT integrated with the host application but rather operate via injection.
    eUseDXGIFactoryProxy = 1 << 5,
    //! Optional - Enables loading of plugins downloaded Over The Air (OTA), to
    //! be used in conjunction with the eAllowOTA flag.
    eLoadDownloadedPlugins = 1 << 6,
};

SL_ENUM_OPERATORS_64(PreferenceFlags)

//! Engine types
//! 
enum class EngineType : uint32_t
{
    eCustom,
    eUnreal,
    eUnity,
    eCount
};

//! Rendering API
//! 
enum class RenderAPI : uint32_t
{
    eD3D11,
    eD3D12,
    eVulkan,
    eCount
};

//! Application preferences
//!
//! {1CA10965-BF8E-432B-8DA1-6716D879FB14}
SL_STRUCT(Preferences, StructType({ 0x1ca10965, 0xbf8e, 0x432b, { 0x8d, 0xa1, 0x67, 0x16, 0xd8, 0x79, 0xfb, 0x14 } }), kStructVersion1)
    //! Optional - In non-production builds it is useful to enable debugging console window
    bool showConsole = false;
    //! Optional - Various logging levels
    LogLevel logLevel = LogLevel::eDefault;
    //! Optional - Absolute paths to locations where to look for plugins, first path in the list has the highest priority
    const wchar_t** pathsToPlugins{};
    //! Optional - Number of paths to search
    uint32_t numPathsToPlugins = 0;
    //! Optional - Absolute path to location where logs and other data should be stored
    //! 
    //! NOTE: Set this to nullptr in order to disable logging to a file
    const wchar_t* pathToLogsAndData{};
    //! Optional - Allows resource allocation tracking on the host side
    PFun_ResourceAllocateCallback* allocateCallback{};
    //! Optional - Allows resource deallocation tracking on the host side
    PFun_ResourceReleaseCallback* releaseCallback{};
    //! Optional - Allows log message tracking including critical errors if they occur
    PFun_LogMessageCallback* logMessageCallback{};
    //! Optional - Flags used to enable or disable advanced options
    PreferenceFlags flags = PreferenceFlags::eDisableCLStateTracking | PreferenceFlags::eAllowOTA | PreferenceFlags::eLoadDownloadedPlugins;
    //! Required - Features to load (assuming appropriate plugins are found), if not specified NO features will be loaded by default
    const Feature* featuresToLoad{};
    //! Required - Number of features to load, only used when list is not a null pointer
    uint32_t numFeaturesToLoad{};
    //! Optional - Id provided by NVIDIA, if not specified then engine type and version are required
    uint32_t applicationId{};
    //! Optional - Type of the rendering engine used, if not specified then applicationId is required
    EngineType engine = EngineType::eCustom;
    //! Optional - Version of the rendering engine used
    const char* engineVersion{};
    //! Optional - GUID (like for example 'a0f57b54-1daf-4934-90ae-c4035c19df04')
    const char* projectId{};
    //! Optional - Which rendering API host is planning to use
    //! 
    //! NOTE: To ensure correct `slGetFeatureRequirements` behavior please specify if planning to use Vulkan.
    RenderAPI renderAPI = RenderAPI::eD3D12;

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};

//! Frame tracking handle
//! 
//! IMPORTANT: Use slGetNewFrameToken to obtain unique instance
//! 
//! {830A0F35-DB84-4171-A804-59B206499B18}
SL_STRUCT_PROTECTED(FrameToken, StructType({ 0x830a0f35, 0xdb84, 0x4171, { 0xa8, 0x4, 0x59, 0xb2, 0x6, 0x49, 0x9b, 0x18 } }), kStructVersion1)
    //! Helper operator to obtain current frame index
    virtual operator uint32_t() const = 0;
};

//! Handle for the unique viewport
//! 
//! {171B6435-9B3C-4FC8-9994-FBE52569AAA4}
SL_STRUCT(ViewportHandle, StructType({ 0x171b6435, 0x9b3c, 0x4fc8, { 0x99, 0x94, 0xfb, 0xe5, 0x25, 0x69, 0xaa, 0xa4 } }), kStructVersion1)
    ViewportHandle(uint32_t v) : BaseStructure(ViewportHandle::s_structType, kStructVersion1), value(v) {}
    ViewportHandle(int32_t v) : BaseStructure(ViewportHandle::s_structType, kStructVersion1), value(v) {}
    operator uint32_t() const { return value; }
private:
    uint32_t value = UINT_MAX;
};

//! Specifies feature requirement flags
//! 
enum class FeatureRequirementFlags : uint32_t
{
    //! Rendering APIs
    eD3D11Supported = 1 << 0,
    eD3D12Supported = 1 << 1,
    eVulkanSupported = 1 << 2,
    //! If set V-Sync must be disabled when feature is active
    eVSyncOffRequired = 1 << 3,
    //! If set GPU hardware scheduling OS feature must be turned on
    eHardwareSchedulingRequired = 1 << 4
};

SL_ENUM_OPERATORS_32(FeatureRequirementFlags);

//! Specifies feature requirements
//! 
//! {66714097-AC6D-4BC6-8915-1E0F55A6B61F}
SL_STRUCT(FeatureRequirements, StructType({ 0x66714097, 0xac6d, 0x4bc6, { 0x89, 0x15, 0x1e, 0xf, 0x55, 0xa6, 0xb6, 0x1f } }), kStructVersion2)
    //! Various Flags
    FeatureRequirementFlags flags {};
    
    //! Feature will create this many CPU threads
    uint32_t maxNumCPUThreads{};

    //! Feature supports only this many viewports
    uint32_t maxNumViewports{};
    
    //! Required buffer tags
    uint32_t numRequiredTags{};
    const BufferType* requiredTags{};

    //! OS and Driver versions
    Version osVersionDetected{};
    Version osVersionRequired{};
    Version driverVersionDetected{};
    Version driverVersionRequired{};

    //! Vulkan specific bits
    
    //! Command queues
    uint32_t vkNumComputeQueuesRequired{};
    uint32_t vkNumGraphicsQueuesRequired{};
    
    //! Device extensions
    uint32_t vkNumDeviceExtensions{};
    const char** vkDeviceExtensions{};
    //! Instance extensions
    uint32_t vkNumInstanceExtensions{};
    const char** vkInstanceExtensions{};
    //! 1.2 features
    //! 
    //! NOTE: Use getVkPhysicalDeviceVulkan12Features from sl_helpers_vk.h
    uint32_t vkNumFeatures12{};
    const char** vkFeatures12{};
    //! 1.3 features
    //! 
    //! NOTE: Use getVkPhysicalDeviceVulkan13Features from sl_helpers_vk.h
    uint32_t vkNumFeatures13{};
    const char** vkFeatures13{};

    //! Vulkan optical flow feature
    uint32_t vkNumOpticalFlowQueuesRequired{};

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};

//! Specifies feature's version
//! 
//! {6D5B51F0-076B-486D-9995-5A561043F5C1}
SL_STRUCT(FeatureVersion, StructType({ 0x6d5b51f0, 0x76b, 0x486d, { 0x99, 0x95, 0x5a, 0x56, 0x10, 0x43, 0xf5, 0xc1 } }), kStructVersion1)
    //! SL version
    Version versionSL{};
    //! NGX version (if feature is using NGX, null otherwise)
    Version versionNGX{};

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};

//! Specifies either DXGI adapter or VK physical device
//! 
//! {0677315F-A746-4492-9F42-CB6142C9C3D4}
SL_STRUCT(AdapterInfo, StructType({ 0x677315f, 0xa746, 0x4492, { 0x9f, 0x42, 0xcb, 0x61, 0x42, 0xc9, 0xc3, 0xd4 } }), kStructVersion1)
    //! Locally unique identifier
    uint8_t* deviceLUID {};
    //! Size in bytes
    uint32_t deviceLUIDSizeInBytes{};
    //! Vulkan Specific, if specified LUID will be ignored
    void* vkPhysicalDevice{};

    //! IMPORTANT: New members go here or if optional can be chained in a new struct, see sl_struct.h for details
};
}