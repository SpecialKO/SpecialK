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

#ifndef ULTRAHDR_MULTIPICTUREFORMAT_H
#define ULTRAHDR_MULTIPICTUREFORMAT_H

#include <memory>

#ifndef USE_BIG_ENDIAN_IN_MPF
#define USE_BIG_ENDIAN_IN_MPF true
#endif

#undef Endian_SwapBE32
#undef Endian_SwapBE16
#if USE_BIG_ENDIAN_IN_MPF
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

constexpr size_t kNumPictures = 2;
constexpr size_t kMpEndianSize = 4;
constexpr uint16_t kTagSerializedCount = 3;
constexpr uint32_t kTagSize = 12;

constexpr uint16_t kTypeLong = 0x4;
constexpr uint16_t kTypeUndefined = 0x7;

static constexpr uint8_t kMpfSig[] = {'M', 'P', 'F', '\0'};
constexpr uint8_t kMpLittleEndian[kMpEndianSize] = {0x49, 0x49, 0x2A, 0x00};
constexpr uint8_t kMpBigEndian[kMpEndianSize] = {0x4D, 0x4D, 0x00, 0x2A};

constexpr uint16_t kVersionTag = 0xB000;
constexpr uint16_t kVersionType = kTypeUndefined;
constexpr uint32_t kVersionCount = 4;
constexpr size_t kVersionSize = 4;
constexpr uint8_t kVersionExpected[kVersionSize] = {'0', '1', '0', '0'};

constexpr uint16_t kNumberOfImagesTag = 0xB001;
constexpr uint16_t kNumberOfImagesType = kTypeLong;
constexpr uint32_t kNumberOfImagesCount = 1;

constexpr uint16_t kMPEntryTag = 0xB002;
constexpr uint16_t kMPEntryType = kTypeUndefined;
constexpr uint32_t kMPEntrySize = 16;

constexpr uint32_t kMPEntryAttributeFormatJpeg = 0x0000000;
constexpr uint32_t kMPEntryAttributeTypePrimary = 0x030000;

size_t calculateMpfSize();
std::shared_ptr<DataStruct> generateMpf(int primary_image_size, int primary_image_offset,
                                        int secondary_image_size, int secondary_image_offset);

}  // namespace ultrahdr

#endif  // ULTRAHDR_MULTIPICTUREFORMAT_H
