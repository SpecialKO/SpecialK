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
#pragma once

#include <SpecialK/render/d3d12/d3d12_interfaces.h>

#pragma pack (push, 1)

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

#pragma pack (pop)