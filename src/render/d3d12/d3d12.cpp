/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#include <SpecialK/stdafx.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Direct3D12"

#define _L2(w)  L ## w
#define  _L(w) _L2(w)

#define D3D12_IGNORE_SDK_LAYERS
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d12/d3d12_interfaces.h>

#include <imgui/backends/imgui_d3d12.h>
#include <SpecialK/render/dxgi/dxgi_hdr.h>

LPVOID                pfnD3D12CreateDevice     = nullptr;
D3D12CreateDevice_pfn D3D12CreateDevice_Import = nullptr;

HMODULE               SK::DXGI::hModD3D12      = nullptr;

volatile LONG         __d3d12_hooked           = FALSE;
volatile LONG         __d3d12_ready            = FALSE;

void
WaitForInitD3D12 (void)
{
  SK_Thread_SpinUntilFlagged (&__d3d12_ready);
}


HRESULT
WINAPI
D3D12CreateDevice_Detour (
  _In_opt_  IUnknown          *pAdapter,
            D3D_FEATURE_LEVEL  MinimumFeatureLevel,
  _In_      REFIID             riid,
  _Out_opt_ void             **ppDevice )
{
  WaitForInitD3D12 ();

  DXGI_LOG_CALL_0 ( L"D3D12CreateDevice" );

  dll_log->LogEx ( true,
                     L"[  D3D 12  ]  <~> Minimum Feature Level - %s\n",
                         SK_DXGI_FeatureLevelsToStr (
                           1,
                             (DWORD *)&MinimumFeatureLevel
                         ).c_str ()
                 );

  if ( pAdapter != nullptr )
  {
    int iver =
      SK_GetDXGIAdapterInterfaceVer ( pAdapter );

    // IDXGIAdapter3 = DXGI 1.4 (Windows 10+)
    if ( iver >= 3 )
    {
      SK::DXGI::StartBudgetThread ( (IDXGIAdapter **)&pAdapter );
    }
  }

  HRESULT res;

  DXGI_CALL (res,
    D3D12CreateDevice_Import ( pAdapter,
                                 MinimumFeatureLevel,
                                   riid,
                                     ppDevice )
  );

  if ( SUCCEEDED ( res ) )
  {
    if ( ppDevice != nullptr )
    {
      //if ( *ppDevice != g_pD3D12Dev )
      //{
        // TODO: This isn't the right way to get the feature level
        dll_log->Log ( L"[  D3D 12  ] >> Device = %ph (Feature Level:%hs)",
                         *ppDevice,
                           SK_DXGI_FeatureLevelsToStr ( 1,
                                                         (DWORD *)&MinimumFeatureLevel//(DWORD *)&ret_level
                                                      ).c_str ()
                     );

        //g_pD3D12Dev =
        //  (IUnknown *)*ppDevice;
      //}
    }
  }

  return res;
}

volatile LONG SK_D3D12_initialized = FALSE;

bool
SK_D3D12_Init (void)
{
  if (SK::DXGI::hModD3D12 == nullptr)
  {
    SK::DXGI::hModD3D12 =
      SK_Modules->LoadLibraryLL (L"d3d12.dll");
  }

  if (SK::DXGI::hModD3D12 != nullptr)
  {
    if ( MH_OK ==
           SK_CreateDLLHook ( L"d3d12.dll",
                               "D3D12CreateDevice",
                                D3D12CreateDevice_Detour,
                     (LPVOID *)&D3D12CreateDevice_Import,
                            &pfnD3D12CreateDevice )
       )
    {
      InterlockedIncrement (&__d3d12_ready);

      return true;
    }
  }

  return false;
}

void
SK_D3D12_Shutdown (void)
{
  if (FALSE == InterlockedCompareExchange (&SK_D3D12_initialized, FALSE, TRUE))
    return;

  SK_FreeLibrary (SK::DXGI::hModD3D12);
}

void
SK_D3D12_EnableHooks (void)
{
  if (pfnD3D12CreateDevice != nullptr)
    SK_EnableHook (pfnD3D12CreateDevice);

  InterlockedIncrement (&__d3d12_hooked);
}

unsigned int
__stdcall
HookD3D12 (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);
#ifndef _WIN64
  return 0;
#else

  if (! config.apis.dxgi.d3d12.hook)
    return 0;

  // This only needs to be done once...
  if (InterlockedCompareExchange (&__d3d12_hooked, TRUE, FALSE)) {
    return 0;
  }

  if (SK::DXGI::hModD3D12 != nullptr)
  {
    //bool success = SUCCEEDED (CoInitializeEx (nullptr, COINIT_MULTITHREADED))

    dll_log->Log (L"[  D3D 12  ]   Hooking D3D12");

#if 0
    SK_ComPtr <IDXGIFactory>  pFactory  = nullptr;
    SK_ComPtr <IDXGIAdapter>  pAdapter  = nullptr;
    SK_ComPtr <IDXGIAdapter1> pAdapter1 = nullptr;

    HRESULT hr =
      CreateDXGIFactory_Import ( IID_PPV_ARGS (&pFactory) );

    if (SUCCEEDED (hr)) {
      pFactory->EnumAdapters (0, &pAdapter);

      if (pFactory) {
        int iver = SK_GetDXGIFactoryInterfaceVer (pFactory);

        SK_ComPtr <IDXGIFactory1> pFactory1 = nullptr;

        if (iver > 0) {
          if (SUCCEEDED (CreateDXGIFactory1_Import ( IID_PPV_ARGS (&pFactory1) ))) {
            pFactory1->EnumAdapters1 (0, &pAdapter1);
          }
        }
      }
    }

    SK_ComPtr <ID3D11Device>        pDevice           = nullptr;
    D3D_FEATURE_LEVEL               featureLevel;
    SK_ComPtr <ID3D11DeviceContext> pImmediateContext = nullptr;

    HRESULT hrx = E_FAIL;
    {
      if (pAdapter1 != nullptr) {
        D3D_FEATURE_LEVEL test_11_1 = D3D_FEATURE_LEVEL_11_1;

        hrx =
          D3D11CreateDevice_Import (
            pAdapter1,
              D3D_DRIVER_TYPE_UNKNOWN,
                nullptr,
                  0,
                    &test_11_1,
                      1,
                        D3D11_SDK_VERSION,
                          &pDevice,
                            &featureLevel,
                              &pImmediateContext );

        if (SUCCEEDED (hrx)) {
          d3d11_caps.feature_level.d3d11_1 = true;
        }
      }

      if (! SUCCEEDED (hrx)) {
        hrx =
          D3D11CreateDevice_Import (
            pAdapter,
              D3D_DRIVER_TYPE_UNKNOWN,
                nullptr,
                  0,
                    nullptr,
                      0,
                        D3D11_SDK_VERSION,
                          &pDevice,
                            &featureLevel,
                              &pImmediateContext );
      }
    }

    if (SUCCEEDED (hrx)) {
    }
#endif
    InterlockedExchange (&__d3d12_ready, TRUE);
  }

  else {
    // Disable this on future runs, because the DLL is not present
    //config.apis.dxgi.d3d12.hook = false;
  }

  return 0;
#endif
}






#pragma pack(push, 1)

static const size_t   DxilContainerHashSize     = 16;
static const uint16_t DxilContainerVersionMajor = 1;          // Current major version
static const uint16_t DxilContainerVersionMinor = 0;          // Current minor version
static const uint32_t DxilContainerMaxSize      = 0x80000000; // Max size for container.

/// Use this type to represent the hash for the full container.
struct DxilContainerHash {
  uint8_t Digest [DxilContainerHashSize];
};

enum class DxilShaderHashFlags : uint32_t {
  None           = 0, // No flags defined.
  IncludesSource = 1, // This flag indicates that the shader hash was computed
                      // taking into account source information (-Zss)
};

typedef struct DxilShaderHash {
  uint32_t Flags; // DxilShaderHashFlags
  uint8_t  Digest [DxilContainerHashSize];
} DxilShaderHash;

struct DxilContainerVersion {
  uint16_t Major;
  uint16_t Minor;
};

/// Use this type to describe a DXIL container of parts.
struct DxilContainerHeader {
  uint32_t             HeaderFourCC;
  DxilContainerHash    Hash;
  DxilContainerVersion Version;
  uint32_t             ContainerSizeInBytes; // From start of this header
  uint32_t             PartCount;
  // Structure is followed by uint32_t PartOffset[PartCount];
  // The offset is to a DxilPartHeader.
};

/// Use this type to describe the size and type of a DXIL container part.
struct DxilPartHeader {
  uint32_t PartFourCC; // Four char code for part type.
  uint32_t PartSize;   // Byte count for PartData.
  // Structure is followed by uint8_t PartData[PartSize].
};

#define DXIL_FOURCC(ch0, ch1, ch2, ch3) (                            \
  (uint32_t)(uint8_t)(ch0)        | (uint32_t)(uint8_t)(ch1) << 8  | \
  (uint32_t)(uint8_t)(ch2) << 16  | (uint32_t)(uint8_t)(ch3) << 24   \
  )

enum DxilFourCC {
  DFCC_Container               = DXIL_FOURCC('D', 'X', 'B', 'C'), // for back-compat with tools that look for DXBC containers
  DFCC_ResourceDef             = DXIL_FOURCC('R', 'D', 'E', 'F'),
  DFCC_InputSignature          = DXIL_FOURCC('I', 'S', 'G', '1'),
  DFCC_OutputSignature         = DXIL_FOURCC('O', 'S', 'G', '1'),
  DFCC_PatchConstantSignature  = DXIL_FOURCC('P', 'S', 'G', '1'),
  DFCC_ShaderStatistics        = DXIL_FOURCC('S', 'T', 'A', 'T'),
  DFCC_ShaderDebugInfoDXIL     = DXIL_FOURCC('I', 'L', 'D', 'B'),
  DFCC_ShaderDebugName         = DXIL_FOURCC('I', 'L', 'D', 'N'),
  DFCC_FeatureInfo             = DXIL_FOURCC('S', 'F', 'I', '0'),
  DFCC_PrivateData             = DXIL_FOURCC('P', 'R', 'I', 'V'),
  DFCC_RootSignature           = DXIL_FOURCC('R', 'T', 'S', '0'),
  DFCC_DXIL                    = DXIL_FOURCC('D', 'X', 'I', 'L'),
  DFCC_PipelineStateValidation = DXIL_FOURCC('P', 'S', 'V', '0'),
  DFCC_RuntimeData             = DXIL_FOURCC('R', 'D', 'A', 'T'),
  DFCC_ShaderHash              = DXIL_FOURCC('H', 'A', 'S', 'H'),
  DFCC_ShaderSourceInfo        = DXIL_FOURCC('S', 'R', 'C', 'I'),
  DFCC_CompilerVersion         = DXIL_FOURCC('V', 'E', 'R', 'S'),
};

#undef DXIL_FOURCC

struct DxilShaderFeatureInfo {
  uint64_t FeatureFlags;
};

// DXIL program information.
struct DxilBitcodeHeader {
  uint32_t DxilMagic;               // ACSII "DXIL".
  uint32_t DxilVersion;             // DXIL version.
  uint32_t BitcodeOffset;           // Offset to LLVM bitcode (from start of header).
  uint32_t BitcodeSize;             // Size of LLVM bitcode.
};
static const uint32_t DxilMagicValue = 0x4C495844; // 'DXIL'

struct DxilProgramHeader {
  uint32_t          ProgramVersion; /// Major and minor version, including type.
  uint32_t          SizeInUint32;   /// Size in uint32_t units including this header.
  DxilBitcodeHeader BitcodeHeader;  /// Bitcode-specific header.
  // Followed by uint8_t[BitcodeHeader.BitcodeOffset]
};

struct DxilProgramSignature {
  uint32_t ParamCount;
  uint32_t ParamOffset;
};

enum class DxilProgramSigMinPrecision : uint32_t {
  Default  = 0U,
  Float16  = 1U,
  Float2_8 = 2U,
  Reserved = 3U,
  SInt16   = 4U,
  UInt16   = 5U,
  Any16    = 0xf0U,
  Any10    = 0xf1U
};

// Corresponds to D3D_NAME and D3D10_SB_NAME
enum class DxilProgramSigSemantic : uint32_t {
  Undefined                  = 0U,
  Position                   = 1U,
  ClipDistance               = 2U,
  CullDistance               = 3U,
  RenderTargetArrayIndex     = 4U,
  ViewPortArrayIndex         = 5U,
  VertexID                   = 6U,
  PrimitiveID                = 7U,
  InstanceID                 = 8U,
  IsFrontFace                = 9U,
  SampleIndex                = 10U,
  FinalQuadEdgeTessfactor    = 11U,
  FinalQuadInsideTessfactor  = 12U,
  FinalTriEdgeTessfactor     = 13U,
  FinalTriInsideTessfactor   = 14U,
  FinalLineDetailTessfactor  = 15U,
  FinalLineDensityTessfactor = 16U,
  Barycentrics               = 23U,
  ShadingRate                = 24U,
  CullPrimitive              = 25U,
  Target                     = 64U,
  Depth                      = 65U,
  Coverage                   = 66U,
  DepthGE                    = 67U,
  DepthLE                    = 68U,
  StencilRef                 = 69U,
  InnerCoverage              = 70U,
};

enum class DxilProgramSigCompType : uint32_t {
  Unknown = 0U,
  UInt32  = 1U,
  SInt32  = 2U,
  Float32 = 3U,
  UInt16  = 4U,
  SInt16  = 5U,
  Float16 = 6U,
  UInt64  = 7U,
  SInt64  = 8U,
  Float64 = 9U,
};

struct DxilProgramSignatureElement {
  uint32_t                   Stream;        // Stream index (parameters must appear in non-decreasing stream order)
  uint32_t                   SemanticName;  // Offset to LPCSTR from start of DxilProgramSignature.
  uint32_t                   SemanticIndex; // Semantic Index
  DxilProgramSigSemantic     SystemValue;   // Semantic type. Similar to DxilSemantic::Kind, but a serialized rather than processing rep.
  DxilProgramSigCompType     CompType;      // Type of bits.
  uint32_t                   Register;      // Register Index (row index)
  uint8_t                    Mask;          // Mask (column allocation)
  union                                     // Unconditional cases useful for validation of shader linkage.
  {
    uint8_t                  NeverWrites_Mask; // For an output signature, the shader the signature belongs to never
                                               // writes the masked components of the output register.
    uint8_t                  AlwaysReads_Mask; // For an input signature, the shader the signature belongs to always
                                               // reads the masked components of the input register.
  };
  uint16_t                   Pad;
  DxilProgramSigMinPrecision MinPrecision;  // Minimum precision of input/output data
};

// Easy to get this wrong. Earlier assertions can help determine
static_assert (sizeof (DxilProgramSignatureElement) == 0x20, "else DxilProgramSignatureElement is misaligned");

struct DxilShaderDebugName {
  uint16_t Flags;       // Reserved, must be set to zero.
  uint16_t NameLength;  // Length of the debug name, without null terminator.
  // Followed by NameLength bytes of the UTF-8-encoded name.
  // Followed by a null terminator.
  // Followed by [0-3] zero bytes to align to a 4-byte boundary.
};
static const size_t MinDxilShaderDebugNameSize = sizeof(DxilShaderDebugName) + 4;

struct DxilCompilerVersion {
  uint16_t Major;
  uint16_t Minor;
  uint32_t VersionFlags;
  uint32_t CommitCount;
  uint32_t VersionStringListSizeInBytes;
  // Followed by VersionStringListSizeInBytes bytes, containing up to two null-terminated strings, sequentially:
  //  1. CommitSha
  //  1. CustomVersionString
  // Followed by [0-3] zero bytes to align to a 4-byte boundary.
};

// Source Info part has the following top level structure:
//
//   DxilSourceInfo
//
//      DxilSourceInfoSection
//         char Data[]
//         (0-3 zero bytes to align to a 4-byte boundary)
//
//      DxilSourceInfoSection
//         char Data[]
//         (0-3 zero bytes to align to a 4-byte boundary)
//
//      ...
//
//      DxilSourceInfoSection
//         char Data[]
//         (0-3 zero bytes to align to a 4-byte boundary)
//
// Each DxilSourceInfoSection is followed by a blob of Data.
// The each type of data has its own internal structure:
//
// ================ 1. Source Names ==================================
//
//  DxilSourceInfo_SourceNames
//
//     DxilSourceInfo_SourceNamesEntry
//        char Name[ NameSizeInBytes ]
//        (0-3 zero bytes to align to a 4-byte boundary)
//
//     DxilSourceInfo_SourceNamesEntry
//        char Name[ NameSizeInBytes ]
//        (0-3 zero bytes to align to a 4-byte boundary)
//
//      ...
//
//     DxilSourceInfo_SourceNamesEntry
//        char Name[ NameSizeInBytes ]
//        (0-3 zero bytes to align to a 4-byte boundary)
//
// ================ 2. Source Contents ==================================
//
//  DxilSourceInfo_SourceContents
//    char Entries[CompressedEntriesSizeInBytes]
//
// `Entries` may be compressed. Here is the uncompressed structure:
//
//     DxilSourceInfo_SourcesContentsEntry
//        char Content[ ContentSizeInBytes ]
//        (0-3 zero bytes to align to a 4-byte boundary)
//
//     DxilSourceInfo_SourcesContentsEntry
//        char Content[ ContentSizeInBytes ]
//        (0-3 zero bytes to align to a 4-byte boundary)
//
//     ...
//
//     DxilSourceInfo_SourcesContentsEntry
//        char Content[ ContentSizeInBytes ]
//        (0-3 zero bytes to align to a 4-byte boundary)
//
// ================ 3. Args ==================================
//
//   DxilSourceInfo_Args
//
//      char ArgName[]; char NullTerm;
//      char ArgValue[]; char NullTerm;
//
//      char ArgName[]; char NullTerm;
//      char ArgValue[]; char NullTerm;
//
//      ...
//
//      char ArgName[]; char NullTerm;
//      char ArgValue[]; char NullTerm;
//

struct DxilSourceInfo {
  uint32_t AlignedSizeInBytes; // Total size of the contents including this header
  uint16_t Flags;              // Reserved, must be set to zero.
  uint16_t SectionCount;       // The number of sections in the source info.
};

enum class DxilSourceInfoSectionType : uint16_t {
  SourceContents = 0ui16,
  SourceNames    = 1ui16,
  Args           = 2ui16,
};

struct DxilSourceInfoSection {
  uint32_t                  AlignedSizeInBytes; // Size of the section, including this header, and the padding. Aligned to 4-byte boundary.
  uint16_t                  Flags;              // Reserved, must be set to zero.
  DxilSourceInfoSectionType Type;               // The type of data following this header.
};

struct DxilSourceInfo_Args {
  uint32_t Flags;                               // Reserved, must be set to zero.
  uint32_t SizeInBytes;                         // Length of all argument pairs, including their null terminators, not including this header.
  uint32_t Count;                               // Number of arguments.

  // Followed by `Count` argument pairs.
  //
  // For example, given the following arguments:
  //    /T ps_6_0 -EMain -D MyDefine=1 /DMyOtherDefine=2 -Zi MyShader.hlsl
  //
  // The argument pair data becomes:
  //    T\0ps_6_0\0
  //    E\0Main\0
  //    D\0MyDefine=1\0
  //    D\0MyOtherDefine=2\0
  //    Zi\0\0
  //    \0MyShader.hlsl\0
  //
};

struct DxilSourceInfo_SourceNames {
  uint32_t Flags;                               // Reserved, must be set to 0.
  uint32_t Count;                               // The number of data entries
  uint16_t EntriesSizeInBytes;                  // The total size of the data entries following this header.

  // Followed by `Count` data entries with the header DxilSourceInfo_SourceNamesEntry
};

struct DxilSourceInfo_SourceNamesEntry {
  uint32_t AlignedSizeInBytes;                  // Size of the data including this header and padding. Aligned to 4-byte boundary.
  uint32_t Flags;                               // Reserved, must be set to 0.
  uint32_t NameSizeInBytes;                     // Size of the file name, *including* the null terminator.
  uint32_t ContentSizeInBytes;                  // Size of the file content, *including* the null terminator.
  // Followed by NameSizeInBytes bytes of the UTF-8-encoded file name (including null terminator).
  // Followed by [0-3] zero bytes to align to a 4-byte boundary.
};

enum class DxilSourceInfo_SourceContentsCompressType : uint16_t {
  None,
  Zlib
};

struct DxilSourceInfo_SourceContents {
  uint32_t AlignedSizeInBytes;                  // Size of the entry including this header. Aligned to 4-byte boundary.
  uint16_t Flags;                               // Reserved, must be set to 0.
  DxilSourceInfo_SourceContentsCompressType
           CompressType;                        // The type of compression used to compress the data
  uint32_t EntriesSizeInBytes;                  // The size of the data entries following this header.
  uint32_t UncompressedEntriesSizeInBytes;      // Total size of the data entries when uncompressed.
  uint32_t Count;                               // The number of data entries
  // Followed by (compressed) `Count` data entries with the header DxilSourceInfo_SourceContentsEntry
};

struct DxilSourceInfo_SourceContentsEntry {
  uint32_t AlignedSizeInBytes;                  // Size of the entry including this header and padding. Aligned to 4-byte boundary.
  uint32_t Flags;                               // Reserved, must be set to 0.
  uint32_t ContentSizeInBytes;                  // Size of the data following this header, *including* the null terminator
  // Followed by ContentSizeInBytes bytes of the UTF-8-encoded content (including null terminator).
  // Followed by [0-3] zero bytes to align to a 4-byte boundary.
};

#pragma pack(pop)










//-----------------------------------------------
IDXGISwapChain3* pLazyD3D12Chain  = nullptr;
ID3D12Device*    pLazyD3D12Device = nullptr;

using
D3D12Device_CreateGraphicsPipelineState_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device*,
                const D3D12_GRAPHICS_PIPELINE_STATE_DESC*,
                      REFIID,void**);
  D3D12Device_CreateGraphicsPipelineState_pfn
  D3D12Device_CreateGraphicsPipelineState_Original = nullptr;

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreateGraphicsPipelineState_Detour (
             ID3D12Device                       *This,
_In_   const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
             REFIID                              riid,
_COM_Outptr_ void                              **ppPipelineState )
{
  HRESULT hrPipelineCreate =
    D3D12Device_CreateGraphicsPipelineState_Original (
      This, pDesc,
      riid, ppPipelineState
    );

  if (pDesc == nullptr)
    return hrPipelineCreate;
                                                                                                  // {4D5298CA-D9F0-6133-A19D-B1D597920000}
static constexpr GUID SKID_D3D12KnownVtxShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x00 } };
static constexpr GUID SKID_D3D12KnownPixShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x01 } };
static constexpr GUID SKID_D3D12KnownGeoShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x02 } };
static constexpr GUID SKID_D3D12KnownHulShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x03 } };
static constexpr GUID SKID_D3D12KnownDomShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x04 } };
static constexpr GUID SKID_D3D12KnownComShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x05 } };
static constexpr GUID SKID_D3D12KnownMshShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x06 } };
static constexpr GUID SKID_D3D12KnownAmpShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x07 } };

  struct shader_repo_s
  {
    struct {
      const char*         name;
      SK_D3D12_ShaderType mask;
    } type;

    struct hash_s {
      struct dxilHashTest
      {
        // std::hash <DxilContainerHash>
        //
        size_t operator ()( const DxilContainerHash& h ) const
        {
          size_t      __h = 0;
          for (size_t __i = 0; __i < DxilContainerHashSize; ++__i)
          {
            __h = h.Digest [__i] +
                    (__h << 06)  +  (__h << 16)
                                 -   __h;
          }

          return __h;
        }

        // std::equal_to <DxilContainerHash>
        //
        bool operator ()( const DxilContainerHash& h1,
                          const DxilContainerHash& h2 ) const
        {
          return
            ( 0 == memcmp ( h1.Digest,
                            h2.Digest, DxilContainerHashSize ) );
        }
      };

      concurrency::concurrent_unordered_set <DxilContainerHash, dxilHashTest, dxilHashTest>
            used;
      GUID  guid;

      hash_s (const GUID& guid_)
      {   memcpy ( &guid,&guid_, sizeof (GUID) );
      };
    } hash;
  } static vertex   { { "Vertex",   SK_D3D12_ShaderType::Vertex   }, { shader_repo_s::hash_s (SKID_D3D12KnownVtxShaderDigest) } },
           pixel    { { "Pixel",    SK_D3D12_ShaderType::Pixel    }, { shader_repo_s::hash_s (SKID_D3D12KnownPixShaderDigest) } },
           geometry { { "Geometry", SK_D3D12_ShaderType::Geometry }, { shader_repo_s::hash_s (SKID_D3D12KnownGeoShaderDigest) } },
           hull     { { "Hull",     SK_D3D12_ShaderType::Hull     }, { shader_repo_s::hash_s (SKID_D3D12KnownHulShaderDigest) } },
           domain   { { "Domain",   SK_D3D12_ShaderType::Domain   }, { shader_repo_s::hash_s (SKID_D3D12KnownDomShaderDigest) } },
           compute  { { "Compute",  SK_D3D12_ShaderType::Compute  }, { shader_repo_s::hash_s (SKID_D3D12KnownComShaderDigest) } },
           mesh     { { "Mesh",     SK_D3D12_ShaderType::Mesh     }, { shader_repo_s::hash_s (SKID_D3D12KnownMshShaderDigest) } },
           amplify  { { "Amplify",  SK_D3D12_ShaderType::Amplify  }, { shader_repo_s::hash_s (SKID_D3D12KnownAmpShaderDigest) } };

  static const
    std::unordered_map <SK_D3D12_ShaderType, shader_repo_s&>
      repo_map =
        { { SK_D3D12_ShaderType::Vertex,   vertex   },
          { SK_D3D12_ShaderType::Pixel,    pixel    },
          { SK_D3D12_ShaderType::Geometry, geometry },
          { SK_D3D12_ShaderType::Hull,     hull     },
          { SK_D3D12_ShaderType::Domain,   domain   },
          { SK_D3D12_ShaderType::Compute,  compute  },
          { SK_D3D12_ShaderType::Mesh,     mesh     },
          { SK_D3D12_ShaderType::Amplify,  amplify  } };

  auto _StashAHash = [&](SK_D3D12_ShaderType type)
  {
    const D3D12_SHADER_BYTECODE
                     *pBytecode = nullptr;

    switch (type)
    {
      case SK_D3D12_ShaderType::Vertex:  pBytecode = (D3D12_SHADER_BYTECODE *)(
        pDesc->VS.BytecodeLength ? pDesc->VS.pShaderBytecode : nullptr); break;
      case SK_D3D12_ShaderType::Pixel:   pBytecode = (D3D12_SHADER_BYTECODE *)(
        pDesc->PS.BytecodeLength ? pDesc->PS.pShaderBytecode : nullptr); break;
      case SK_D3D12_ShaderType::Geometry:pBytecode = (D3D12_SHADER_BYTECODE *)(
        pDesc->GS.BytecodeLength ? pDesc->GS.pShaderBytecode : nullptr); break;
      case SK_D3D12_ShaderType::Domain:  pBytecode = (D3D12_SHADER_BYTECODE *)(
        pDesc->DS.BytecodeLength ? pDesc->DS.pShaderBytecode : nullptr); break;
      case SK_D3D12_ShaderType::Hull:    pBytecode = (D3D12_SHADER_BYTECODE *)(
        pDesc->HS.BytecodeLength ? pDesc->HS.pShaderBytecode : nullptr); break;
      default:                           pBytecode = nullptr;            break;
    }

    if (pBytecode != nullptr/* && repo_map.count (type)*/)
    {
      auto FourCC =  ((DxilContainerHeader *)pBytecode)->HeaderFourCC;
      auto pHash  = &((DxilContainerHeader *)pBytecode)->Hash.Digest [0];

      if ( FourCC == DFCC_Container || FourCC == DFCC_DXIL )
      {
        shader_repo_s& repo =
          repo_map.at (type);

        if (repo.hash.used.insert (((DxilContainerHeader *)pBytecode)->Hash).second)
        {
          SK_LOG0 ( ( L"%9hs Shader (BytecodeType=%s) [%02x%02x%02x%02x%02x%02x%02x%02x"
                                                     L"%02x%02x%02x%02x%02x%02x%02x%02x]",
                      repo.type.name, FourCC ==
                                        DFCC_Container ?
                                               L"DXBC" : L"DXIL",
              pHash [ 0], pHash [ 1], pHash [ 2], pHash [ 3], pHash [ 4], pHash [ 5],
              pHash [ 6], pHash [ 7], pHash [ 8], pHash [ 9], pHash [10], pHash [11],
              pHash [12], pHash [13], pHash [14], pHash [15] ),
                    __SK_SUBSYSTEM__ );
        }

        //SK_ComQIPtr <ID3D12Object>
        //    pPipelineState ((IUnknown *)*ppPipelineState);
        //if (pPipelineState != nullptr)
        //{
        //  ////pPipelineState->SetPrivateData ( repo.hash.guid,
        //  ////                         DxilContainerHashSize,
        //  ////                                     pHash );
        //}
      }

      else
      {
        shader_repo_s& repo =
          repo_map.at (type);

        SK_LOG0 ( ( L"%9hs Shader (FourCC=%hs,%lu)", repo.type.name, (char *)&FourCC, FourCC ), __SK_SUBSYSTEM__ );
      }
    }
  };

  if (SUCCEEDED (hrPipelineCreate))
  {
    if (pDesc->VS.pShaderBytecode) _StashAHash (SK_D3D12_ShaderType::Vertex);
    if (pDesc->PS.pShaderBytecode) _StashAHash (SK_D3D12_ShaderType::Pixel);
    if (pDesc->GS.pShaderBytecode) _StashAHash (SK_D3D12_ShaderType::Geometry);
    if (pDesc->HS.pShaderBytecode) _StashAHash (SK_D3D12_ShaderType::Hull);
    if (pDesc->DS.pShaderBytecode) _StashAHash (SK_D3D12_ShaderType::Domain);
  }

  return
    hrPipelineCreate;
}

using
D3D12Device_CreateRenderTargetView_pfn = void
(STDMETHODCALLTYPE *)(ID3D12Device*,ID3D12Resource*,
                const D3D12_RENDER_TARGET_VIEW_DESC*,
                      D3D12_CPU_DESCRIPTOR_HANDLE);
  D3D12Device_CreateRenderTargetView_pfn
  D3D12Device_CreateRenderTargetView_Original = nullptr;

void
STDMETHODCALLTYPE
D3D12Device_CreateRenderTargetView_Detour (
                ID3D12Device                  *This,
_In_opt_        ID3D12Resource                *pResource,
_In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
_In_            D3D12_CPU_DESCRIPTOR_HANDLE    DestDescriptor )
{
  if (pDesc != nullptr)
  {
    if ( __SK_HDR_16BitSwap &&
           pDesc->ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2D )
    {
      if (pResource != nullptr)
      {
        auto desc =
          pResource->GetDesc ();

        if ( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
             desc.Format    == DXGI_FORMAT_R16G16B16A16_FLOAT )
        {
          if (                       pDesc->Format  != DXGI_FORMAT_UNKNOWN &&
              DirectX::MakeTypeless (pDesc->Format) != DXGI_FORMAT_R16G16B16A16_TYPELESS)
          {
            SK_LOG_FIRST_CALL

            auto fixed_desc = *pDesc;
                 fixed_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

            return
              D3D12Device_CreateRenderTargetView_Original ( This,
                pResource, &fixed_desc,
                  DestDescriptor
              );
          }
        }
      }
    }
  }

  return
    D3D12Device_CreateRenderTargetView_Original (This, pResource, pDesc, DestDescriptor);
}

using
D3D12Device_CreateCommittedResource_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device*,const D3D12_HEAP_PROPERTIES*,D3D12_HEAP_FLAGS,
                                    const D3D12_RESOURCE_DESC*,D3D12_RESOURCE_STATES,
                                    const D3D12_CLEAR_VALUE*,REFIID,void**);
D3D12Device_CreateCommittedResource_pfn
D3D12Device_CreateCommittedResource_Original = nullptr;

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreateCommittedResource_Detour (
                 ID3D12Device           *This,
_In_       const D3D12_HEAP_PROPERTIES  *pHeapProperties,
                 D3D12_HEAP_FLAGS        HeapFlags,
_In_       const D3D12_RESOURCE_DESC    *pDesc,
                 D3D12_RESOURCE_STATES   InitialResourceState,
_In_opt_   const D3D12_CLEAR_VALUE      *pOptimizedClearValue,
                 REFIID                  riidResource,
_COM_Outptr_opt_ void                  **ppvResource )
{
  D3D12_RESOURCE_DESC _desc = *pDesc;

  if (SK_GetCurrentGameID () == SK_GAME_ID::EldenRing)
  {
    if (pDesc->Alignment == 4096)// && pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
        _desc.Alignment = 0;

    if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
      if (pDesc->Alignment == 4096)
      {
        SK_RunOnce (SK_ImGui_Warning (L"Gotcha!"));

        _desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
      }
    }
  }

  return
    D3D12Device_CreateCommittedResource_Original ( This,
      pHeapProperties, HeapFlags, &_desc/*pDesc*/, InitialResourceState,
        pOptimizedClearValue, riidResource, ppvResource );
}

using
D3D12Device_CreatePlacedResource_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device*,ID3D12Heap*,
                       UINT64,const D3D12_RESOURCE_DESC*,
                                    D3D12_RESOURCE_STATES,
                              const D3D12_CLEAR_VALUE*,REFIID,void**);

D3D12Device_CreatePlacedResource_pfn
D3D12Device_CreatePlacedResource_Original = nullptr;

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreatePlacedResource_Detour (
                 ID3D12Device           *This,
_In_             ID3D12Heap             *pHeap,
                 UINT64                  HeapOffset,
_In_       const D3D12_RESOURCE_DESC    *pDesc,
                 D3D12_RESOURCE_STATES   InitialState,
_In_opt_   const D3D12_CLEAR_VALUE      *pOptimizedClearValue,
                 REFIID                  riid,
_COM_Outptr_opt_ void                  **ppvResource )
{
  D3D12_RESOURCE_DESC _desc = *pDesc;

  if (SK_GetCurrentGameID () == SK_GAME_ID::EldenRing)
  {
    if (pDesc->Alignment == 4096)// && pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
         _desc.Alignment = 0;

    if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
      if (pDesc->Alignment == 4096)
      {
        SK_RunOnce (SK_ImGui_Warning (L"Gotcha!"));

        _desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
      }
    }
  }

  return
    D3D12Device_CreatePlacedResource_Original ( This,
      pHeap, HeapOffset, &_desc/*pDesc*/, InitialState,
        pOptimizedClearValue, riid, ppvResource );
}

//virtual HRESULT STDMETHODCALLTYPE CreateReservedResource( 
//    _In_  const D3D12_RESOURCE_DESC *pDesc,
//    D3D12_RESOURCE_STATES InitialState,
//    _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
//    REFIID riid,
//    _COM_Outptr_opt_  void **ppvResource) = 0;

using
D3D12CommandQueue_ExecuteCommandLists_pfn = void
(STDMETHODCALLTYPE *)(ID3D12CommandQueue*, UINT,
                      ID3D12CommandList* const *);
   D3D12CommandQueue_ExecuteCommandLists_pfn
   D3D12CommandQueue_ExecuteCommandLists_Original = nullptr;

void STDMETHODCALLTYPE
D3D12CommandQueue_ExecuteCommandLists_Detour (
  ID3D12CommandQueue        *This,
  UINT                      NumCommandLists,
  ID3D12CommandList* const  *ppCommandLists )
{
  static bool          once = false;
  if (! std::exchange (once,  true))
  {
    static auto& rb =
      SK_GetCurrentRenderBackend ();

    SK_ComPtr <ID3D12Device> pDevice12;

    D3D12_COMMAND_QUEUE_DESC
         queueDesc  =  This->GetDesc ();
    if ( queueDesc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT &&
         SUCCEEDED (   This->GetDevice (
                         IID_ID3D12Device,
                        (void **)&pDevice12.p
                                       ) // We are not holding a ref, so test pointers before using
                   ) && SK_ValidatePointer                         (pLazyD3D12Device,      true) &&
                        SK_IsAddressExecutable ((*(const void***)*(&pLazyD3D12Device))[0], true) &&
                        pDevice12.IsEqualObject (
               pLazyD3D12Device                 )
       )
    {
      if (rb.d3d12.command_queue == nullptr)
      {
        // Now we are holding a ref...
        rb.setDevice            (pLazyD3D12Device);
        rb.swapchain           = pLazyD3D12Chain;
        rb.d3d12.command_queue = This;
        rb.api                 = SK_RenderAPI::D3D12;

        _d3d12_rbk->init (
          (IDXGISwapChain3 *)pLazyD3D12Chain,
            This
        );
      }
    }

    else once = false;
  }


  if (once)
  {
    if ( ReadULong64Acquire (&SK_Reflex_LastFrameMarked) <
         ReadULong64Acquire (&SK_RenderBackend::frames_drawn) )
    {
      static auto& rb =
       SK_GetCurrentRenderBackend ();

      WriteULong64Release (
        &SK_Reflex_LastFrameMarked,
         ReadULong64Acquire (&SK_RenderBackend::frames_drawn)
      );

      rb.setLatencyMarkerNV (RENDERSUBMIT_START);
    }
  }


  return
    D3D12CommandQueue_ExecuteCommandLists_Original (
      This,
        NumCommandLists,
         ppCommandLists
    );
}

bool
SK_D3D12_HotSwapChainHook ( IDXGISwapChain3* pSwapChain,
                            ID3D12Device*    pDev12 )
{
  static bool                                               init = false;
  if ((! std::exchange (pLazyD3D12Chain, pSwapChain)) || (! init))
  {                     pLazyD3D12Device = pDev12;
    //
    // HACK for HDR RenderTargetView issues in D3D12
    //
    if (     pDev12                                  != nullptr &&
         D3D12Device_CreateRenderTargetView_Original == nullptr )
    {
      // TODO:  Initialize this somewhere else  (earlier !!)
      //
      SK_CreateVFTableHook2 ( L"ID3D12Device::CreateGraphicsPipelineState",
                               *(void ***)*(&pDev12), 10,
                                D3D12Device_CreateGraphicsPipelineState_Detour,
                      (void **)&D3D12Device_CreateGraphicsPipelineState_Original );

      SK_CreateVFTableHook2 ( L"ID3D12Device::CreateRenderTargetView",
                               *(void ***)*(&pDev12), 20,
                                D3D12Device_CreateRenderTargetView_Detour,
                      (void **)&D3D12Device_CreateRenderTargetView_Original );

      SK_CreateVFTableHook2 ( L"ID3D12Device::CreateCommittedResource",
                               *(void ***)*(&pDev12), 27,
                                D3D12Device_CreateCommittedResource_Detour,
                      (void **)&D3D12Device_CreateCommittedResource_Original );

      SK_CreateVFTableHook2 ( L"ID3D12Device::CreatePlacedResource",
                               *(void ***)*(&pDev12), 29,
                                D3D12Device_CreatePlacedResource_Detour,
                      (void **)&D3D12Device_CreatePlacedResource_Original );

      // 21 CreateDepthStencilView
      // 22 CreateSampler
      // 23 CopyDescriptors
      // 24 CopyDescriptorsSimple
      // 25 GetResourceAllocationInfo
      // 26 GetCustomHeapProperties
      // 27 CreateCommittedResource
      // 28 CreateHeap
      // 29 CreatePlacedResource
      // 30 CreateReservedResource
    }

    SK_ComPtr < ID3D12CommandQueue > p12Queue;
    D3D12_COMMAND_QUEUE_DESC queue_desc = { };

    if (SUCCEEDED (pDev12->CreateCommandQueue (
                            &queue_desc,
                      IID_PPV_ARGS (&p12Queue.p))))
    {
      SK_CreateVFTableHook2 ( L"ID3D12CommandQueue::ExecuteCommandLists",
                               *(void ***)*(&p12Queue.p), 10,
                                D3D12CommandQueue_ExecuteCommandLists_Detour,
                      (void **)&D3D12CommandQueue_ExecuteCommandLists_Original );

      SK_ApplyQueuedHooks ();

      init = true;
    }
  }

  return init;
}

extern void
SK_D3D11_SetPipelineStats (void* pData);

void
__stdcall
SK_D3D12_UpdateRenderStats (IDXGISwapChain* pSwapChain)
{
  if (pSwapChain == nullptr || (! config.render.show))
    return;

  // Need more debug time with D3D12
  return;

#if 0
  SK_ComPtr <ID3D12Device> dev = nullptr;

  if (SUCCEEDED (pSwapChain->GetDevice (IID_PPV_ARGS (&dev)))) {
    static SK_ComPtr <ID3D12CommandAllocator>    cmd_alloc = nullptr;
    static SK_ComPtr <ID3D12GraphicsCommandList> cmd_list  = nullptr;
    static SK_ComPtr <ID3D12Resource>            query_res = nullptr;
    static SK_ComPtr <ID3D12CommandQueue>        cmd_queue = nullptr;

    if (cmd_alloc == nullptr)
    {
      dev->CreateCommandAllocator ( D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      IID_PPV_ARGS (&cmd_alloc) );

      D3D12_COMMAND_QUEUE_DESC queue_desc = { };

      queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
      queue_desc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;

      dev->CreateCommandQueue ( &queue_desc,
                                  IID_PPV_ARGS (&cmd_queue) );

      if (query_res == nullptr) {
        D3D12_HEAP_PROPERTIES heap_props = { D3D12_HEAP_TYPE_READBACK,
                                             D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                             D3D12_MEMORY_POOL_UNKNOWN,
                                             0xFF,
                                             0xFF };

        D3D12_RESOURCE_DESC res_desc = { };
        res_desc.Width     = sizeof D3D12_QUERY_DATA_PIPELINE_STATISTICS;
        res_desc.Flags     = D3D12_RESOURCE_FLAG_NONE;
        res_desc.Alignment = 0;

        dev->CreateCommittedResource ( &heap_props,
                                       D3D12_HEAP_FLAG_NONE,
                                       &res_desc,
                                       D3D12_RESOURCE_STATE_COPY_DEST,
                                       nullptr,
                                       IID_PPV_ARGS (&query_res) );
      }
    }

    if (cmd_alloc != nullptr && cmd_list == nullptr)
    {
      dev->CreateCommandList ( 0,
                                 D3D12_COMMAND_LIST_TYPE_DIRECT,
                                   cmd_alloc,
                                     nullptr,
                                       IID_PPV_ARGS (&cmd_list) );
    }

    if (cmd_alloc == nullptr)
      return;

    SK::DXGI::PipelineStatsD3D12& pipeline_stats =
      SK::DXGI::pipeline_stats_d3d11;

    if (pipeline_stats.query.heap != nullptr) {
      if (pipeline_stats.query.active) {
        cmd_list->EndQuery (pipeline_stats.query.heap, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0);
        cmd_list->Close    ();

        cmd_queue->ExecuteCommandLists (1, (ID3D12CommandList * const*)&cmd_list);

        //dev_ctx->End (pipeline_stats.query.heap[);
        pipeline_stats.query.active = false;
      } else {
        cmd_list->ResolveQueryData ( pipeline_stats.query.heap,
                                       D3D12_QUERY_TYPE_PIPELINE_STATISTICS,
                                         0, 1,
                                           query_res,
                                             0 );

        if (true) {//hr == S_OK) {
          pipeline_stats.query.heap->Release ();
          pipeline_stats.query.heap = nullptr;

          cmd_list.Release ();
          cmd_list = nullptr;

          void *pData = nullptr;

          D3D12_RANGE range = { 0, sizeof D3D12_QUERY_DATA_PIPELINE_STATISTICS };

          query_res->Map (0, &range, &pData);

          SK_D3D11_SetPipelineStats (pData);

          static const D3D12_RANGE unmap_range = { 0, 0 };

          query_res->Unmap (0, &unmap_range);
        }
      }
    }

    else {
      D3D12_QUERY_HEAP_DESC query_heap_desc {
        D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS, 1, 0xFF /* 255 GPUs seems like enough? :P */
      };

      if (SUCCEEDED (dev->CreateQueryHeap (&query_heap_desc, IID_PPV_ARGS (&pipeline_stats.query.heap)))) {
        cmd_list->BeginQuery (pipeline_stats.query.heap, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0);
        cmd_list->Close      ();

        cmd_queue->ExecuteCommandLists (1, (ID3D12CommandList * const*)&cmd_list);
        //dev->BeginQuery (Begin (pipeline_stats.query.heap);
        pipeline_stats.query.active = true;
      }
    }
  }
#endif
}