/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ULTRAHDR_ICC_H
#define ULTRAHDR_ICC_H

#include <memory>

#ifndef USE_BIG_ENDIAN_IN_ICC
#define USE_BIG_ENDIAN_IN_ICC true
#endif

#undef Endian_SwapBE32
#undef Endian_SwapBE16
#if USE_BIG_ENDIAN_IN_ICC
#define Endian_SwapBE32(n) EndianSwap32(n)
#define Endian_SwapBE16(n) EndianSwap16(n)
#else
#define Endian_SwapBE32(n) (n)
#define Endian_SwapBE16(n) (n)
#endif

#include "ultrahdr/jpegr.h"
#include "ultrahdr/gainmapmath.h"
#include "ultrahdr/jpegrutils.h"

namespace ultrahdr {

typedef int32_t Fixed;
#define Fixed1 (1 << 16)
#define MaxS32FitsInFloat 2147483520
#define MinS32FitsInFloat (-MaxS32FitsInFloat)
#define FixedToFloat(x) ((x)*1.52587890625e-5f)

typedef struct Matrix3x3 {
  float vals[3][3];
} Matrix3x3;

// The D50 illuminant.
constexpr float kD50_x = 0.9642f;
constexpr float kD50_y = 1.0000f;
constexpr float kD50_z = 0.8249f;

enum {
  // data_color_space
  Signature_CMYK = 0x434D594B,
  Signature_Gray = 0x47524159,
  Signature_RGB = 0x52474220,

  // pcs
  Signature_Lab = 0x4C616220,
  Signature_XYZ = 0x58595A20,
};

typedef uint32_t FourByteTag;
static inline constexpr FourByteTag SetFourByteTag(char a, char b, char c, char d) {
  return (((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d);
}

static constexpr char kICCIdentifier[] = "ICC_PROFILE";
// 12 for the actual identifier, +2 for the chunk count and chunk index which
// will always follow.
static constexpr size_t kICCIdentifierSize = 14;

// This is equal to the header size according to the ICC specification (128)
// plus the size of the tag count (4).  We include the tag count since we
// always require it to be present anyway.
static constexpr size_t kICCHeaderSize = 132;

// Contains a signature (4), offset (4), and size (4).
static constexpr size_t kICCTagTableEntrySize = 12;

// size should be 20; 4 bytes for type descriptor, 4 bytes reserved, 12
// bytes for a single XYZ number type (4 bytes per coordinate).
static constexpr size_t kColorantTagSize = 20;

static constexpr uint32_t kDisplay_Profile = SetFourByteTag('m', 'n', 't', 'r');
static constexpr uint32_t kRGB_ColorSpace = SetFourByteTag('R', 'G', 'B', ' ');
static constexpr uint32_t kXYZ_PCSSpace = SetFourByteTag('X', 'Y', 'Z', ' ');
static constexpr uint32_t kACSP_Signature = SetFourByteTag('a', 'c', 's', 'p');

static constexpr uint32_t kTAG_desc = SetFourByteTag('d', 'e', 's', 'c');
static constexpr uint32_t kTAG_TextType = SetFourByteTag('m', 'l', 'u', 'c');
static constexpr uint32_t kTAG_rXYZ = SetFourByteTag('r', 'X', 'Y', 'Z');
static constexpr uint32_t kTAG_gXYZ = SetFourByteTag('g', 'X', 'Y', 'Z');
static constexpr uint32_t kTAG_bXYZ = SetFourByteTag('b', 'X', 'Y', 'Z');
static constexpr uint32_t kTAG_wtpt = SetFourByteTag('w', 't', 'p', 't');
static constexpr uint32_t kTAG_rTRC = SetFourByteTag('r', 'T', 'R', 'C');
static constexpr uint32_t kTAG_gTRC = SetFourByteTag('g', 'T', 'R', 'C');
static constexpr uint32_t kTAG_bTRC = SetFourByteTag('b', 'T', 'R', 'C');
static constexpr uint32_t kTAG_cicp = SetFourByteTag('c', 'i', 'c', 'p');
static constexpr uint32_t kTAG_cprt = SetFourByteTag('c', 'p', 'r', 't');
static constexpr uint32_t kTAG_A2B0 = SetFourByteTag('A', '2', 'B', '0');
static constexpr uint32_t kTAG_B2A0 = SetFourByteTag('B', '2', 'A', '0');

static constexpr uint32_t kTAG_CurveType = SetFourByteTag('c', 'u', 'r', 'v');
static constexpr uint32_t kTAG_mABType = SetFourByteTag('m', 'A', 'B', ' ');
static constexpr uint32_t kTAG_mBAType = SetFourByteTag('m', 'B', 'A', ' ');
static constexpr uint32_t kTAG_ParaCurveType = SetFourByteTag('p', 'a', 'r', 'a');

static constexpr Matrix3x3 kSRGB = {{
    // ICC fixed-point (16.16) representation, taken from skcms. Please keep them exactly in sync.
    // 0.436065674f, 0.385147095f, 0.143066406f,
    // 0.222488403f, 0.716873169f, 0.060607910f,
    // 0.013916016f, 0.097076416f, 0.714096069f,
    {FixedToFloat(0x6FA2), FixedToFloat(0x6299), FixedToFloat(0x24A0)},
    {FixedToFloat(0x38F5), FixedToFloat(0xB785), FixedToFloat(0x0F84)},
    {FixedToFloat(0x0390), FixedToFloat(0x18DA), FixedToFloat(0xB6CF)},
}};

static constexpr Matrix3x3 kDisplayP3 = {{
    {0.515102f, 0.291965f, 0.157153f},
    {0.241182f, 0.692236f, 0.0665819f},
    {-0.00104941f, 0.0418818f, 0.784378f},
}};

static constexpr Matrix3x3 kRec2020 = {{
    {0.673459f, 0.165661f, 0.125100f},
    {0.279033f, 0.675338f, 0.0456288f},
    {-0.00193139f, 0.0299794f, 0.797162f},
}};

static constexpr uint32_t kCICPPrimariesSRGB = 1;
static constexpr uint32_t kCICPPrimariesP3 = 12;
static constexpr uint32_t kCICPPrimariesRec2020 = 9;

static constexpr uint32_t kCICPTrfnSRGB = 1;
static constexpr uint32_t kCICPTrfnLinear = 8;
static constexpr uint32_t kCICPTrfnPQ = 16;
static constexpr uint32_t kCICPTrfnHLG = 18;

enum ParaCurveType {
  kExponential_ParaCurveType = 0,
  kGAB_ParaCurveType = 1,
  kGABC_ParaCurveType = 2,
  kGABDE_ParaCurveType = 3,
  kGABCDEF_ParaCurveType = 4,
};

/**
 *  Return the closest int for the given float. Returns MaxS32FitsInFloat for NaN.
 */
static inline int float_saturate2int(float x) {
  x = x < MaxS32FitsInFloat ? x : MaxS32FitsInFloat;
  x = x > MinS32FitsInFloat ? x : MinS32FitsInFloat;
  return (int)x;
}

static inline Fixed float_round_to_fixed(float x) {
  return float_saturate2int((float)floor((double)x * Fixed1 + 0.5));
}

static inline uint16_t float_round_to_unorm16(float x) {
  x = x * 65535.f + 0.5;
  if (x > 65535) return 65535;
  if (x < 0) return 0;
  return static_cast<uint16_t>(x);
}

static inline void float_to_table16(const float f, uint8_t* table_16) {
  *reinterpret_cast<uint16_t*>(table_16) = Endian_SwapBE16(float_round_to_unorm16(f));
}

static inline bool isfinitef_(float x) { return 0 == x * 0; }

struct ICCHeader {
  // Size of the profile (computed)
  uint32_t size;
  // Preferred CMM type (ignored)
  uint32_t cmm_type = 0;
  // Version 4.3 or 4.4 if CICP is included.
  uint32_t version = Endian_SwapBE32(0x04300000);
  // Display device profile
  uint32_t profile_class = Endian_SwapBE32(kDisplay_Profile);
  // RGB input color space;
  uint32_t data_color_space = Endian_SwapBE32(kRGB_ColorSpace);
  // Profile connection space.
  uint32_t pcs = Endian_SwapBE32(kXYZ_PCSSpace);
  // Date and time (ignored)
  uint8_t creation_date_time[12] = {0};
  // Profile signature
  uint32_t signature = Endian_SwapBE32(kACSP_Signature);
  // Platform target (ignored)
  uint32_t platform = 0;
  // Flags: not embedded, can be used independently
  uint32_t flags = 0x00000000;
  // Device manufacturer (ignored)
  uint32_t device_manufacturer = 0;
  // Device model (ignored)
  uint32_t device_model = 0;
  // Device attributes (ignored)
  uint8_t device_attributes[8] = {0};
  // Relative colorimetric rendering intent
  uint32_t rendering_intent = Endian_SwapBE32(1);
  // D50 standard illuminant (X, Y, Z)
  uint32_t illuminant_X = Endian_SwapBE32(float_round_to_fixed(kD50_x));
  uint32_t illuminant_Y = Endian_SwapBE32(float_round_to_fixed(kD50_y));
  uint32_t illuminant_Z = Endian_SwapBE32(float_round_to_fixed(kD50_z));
  // Profile creator (ignored)
  uint32_t creator = 0;
  // Profile id checksum (ignored)
  uint8_t profile_id[16] = {0};
  // Reserved (ignored)
  uint8_t reserved[28] = {0};
  // Technically not part of header, but required
  uint32_t tag_count = 0;
};

class IccHelper {
 private:
  static constexpr uint32_t kTrcTableSize = 65;
  static constexpr uint32_t kGridSize = 17;
  static constexpr size_t kNumChannels = 3;

  static std::shared_ptr<DataStruct> write_text_tag(const char* text);
  static std::string get_desc_string(const uhdr_color_transfer_t tf,
                                     const uhdr_color_gamut_t gamut);
  static std::shared_ptr<DataStruct> write_xyz_tag(float x, float y, float z);
  static std::shared_ptr<DataStruct> write_trc_tag(const int table_entries, const void* table_16);
  static std::shared_ptr<DataStruct> write_trc_tag(const TransferFunction& fn);
  static float compute_tone_map_gain(const uhdr_color_transfer_t tf, float L);
  static std::shared_ptr<DataStruct> write_cicp_tag(uint32_t color_primaries,
                                                    uint32_t transfer_characteristics);
  static std::shared_ptr<DataStruct> write_mAB_or_mBA_tag(uint32_t type, bool has_a_curves,
                                                          const uint8_t* grid_points,
                                                          const uint8_t* grid_16);
  static void compute_lut_entry(const Matrix3x3& src_to_XYZD50, float rgb[3]);
  static std::shared_ptr<DataStruct> write_clut(const uint8_t* grid_points, const uint8_t* grid_16);

  // Checks if a set of xyz tags is equivalent to a 3x3 Matrix. Each input
  // tag buffer assumed to be at least kColorantTagSize in size.
  static bool tagsEqualToMatrix(const Matrix3x3& matrix, const uint8_t* red_tag,
                                const uint8_t* green_tag, const uint8_t* blue_tag);

 public:
  // Output includes JPEG embedding identifier and chunk information, but not
  // APPx information.
  static std::shared_ptr<DataStruct> writeIccProfile(const uhdr_color_transfer_t tf,
                                                     const uhdr_color_gamut_t gamut);
  // NOTE: this function is not robust; it can infer gamuts that IccHelper
  // writes out but should not be considered a reference implementation for
  // robust parsing of ICC profiles or their gamuts.
  static uhdr_color_gamut_t readIccColorGamut(void* icc_data, size_t icc_size);
};

}  // namespace ultrahdr

#endif  // ULTRAHDR_ICC_H
