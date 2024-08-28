/*
 * Copyright 2024 The Android Open Source Project
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

#ifndef ULTRAHDR_GAINMAPMETADATA_H
#define ULTRAHDR_GAINMAPMETADATA_H

#include "ultrahdr/ultrahdrcommon.h"

#include <memory>
#include <vector>

namespace ultrahdr {

// Gain map metadata, for tone mapping between SDR and HDR.
// This is the fraction version of {@code uhdr_gainmap_metadata_ext_t}.
struct uhdr_gainmap_metadata_frac {
  uint32_t gainMapMinN[3];
  uint32_t gainMapMinD[3];
  uint32_t gainMapMaxN[3];
  uint32_t gainMapMaxD[3];
  uint32_t gainMapGammaN[3];
  uint32_t gainMapGammaD[3];

  uint32_t baseOffsetN[3];
  uint32_t baseOffsetD[3];
  uint32_t alternateOffsetN[3];
  uint32_t alternateOffsetD[3];

  uint32_t baseHdrHeadroomN;
  uint32_t baseHdrHeadroomD;
  uint32_t alternateHdrHeadroomN;
  uint32_t alternateHdrHeadroomD;

  bool backwardDirection;
  bool useBaseColorSpace;

  static uhdr_error_info_t encodeGainmapMetadata(const uhdr_gainmap_metadata_frac* in_metadata,
                                                 std::vector<uint8_t>& out_data);

  static uhdr_error_info_t decodeGainmapMetadata(const std::vector<uint8_t>& in_data,
                                                 uhdr_gainmap_metadata_frac* out_metadata);

  static uhdr_error_info_t gainmapMetadataFractionToFloat(const uhdr_gainmap_metadata_frac* from,
                                                          uhdr_gainmap_metadata_ext_t* to);

  static uhdr_error_info_t gainmapMetadataFloatToFraction(const uhdr_gainmap_metadata_ext_t* from,
                                                          uhdr_gainmap_metadata_frac* to);

  void dump() const {
    ALOGD("GAIN MAP METADATA: \n");
    ALOGD("min numerator:                       %d, %d, %d\n", gainMapMinN[0], gainMapMinN[1],
          gainMapMinN[2]);
    ALOGD("min denominator:                     %d, %d, %d\n", gainMapMinD[0], gainMapMinD[1],
          gainMapMinD[2]);
    ALOGD("max numerator:                       %d, %d, %d\n", gainMapMaxN[0], gainMapMaxN[1],
          gainMapMaxN[2]);
    ALOGD("max denominator:                     %d, %d, %d\n", gainMapMaxD[0], gainMapMaxD[1],
          gainMapMaxD[2]);
    ALOGD("gamma numerator:                     %d, %d, %d\n", gainMapGammaN[0], gainMapGammaN[1],
          gainMapGammaN[2]);
    ALOGD("gamma denominator:                   %d, %d, %d\n", gainMapGammaD[0], gainMapGammaD[1],
          gainMapGammaD[2]);
    ALOGD("SDR offset numerator:                %d, %d, %d\n", baseOffsetN[0], baseOffsetN[1],
          baseOffsetN[2]);
    ALOGD("SDR offset denominator:              %d, %d, %d\n", baseOffsetD[0], baseOffsetD[1],
          baseOffsetD[2]);
    ALOGD("HDR offset numerator:                %d, %d, %d\n", alternateOffsetN[0],
          alternateOffsetN[1], alternateOffsetN[2]);
    ALOGD("HDR offset denominator:              %d, %d, %d\n", alternateOffsetD[0],
          alternateOffsetD[1], alternateOffsetD[2]);
    ALOGD("base HDR head room numerator:        %d\n", baseHdrHeadroomN);
    ALOGD("base HDR head room denominator:      %d\n", baseHdrHeadroomD);
    ALOGD("alternate HDR head room numerator:   %d\n", alternateHdrHeadroomN);
    ALOGD("alternate HDR head room denominator: %d\n", alternateHdrHeadroomD);
    ALOGD("backwardDirection:                   %s\n", backwardDirection ? "true" : "false");
    ALOGD("use base color space:                %s\n", useBaseColorSpace ? "true" : "false");
  }
};

}  // namespace ultrahdr

#endif  // ULTRAHDR_GAINMAPMETADATA_H
