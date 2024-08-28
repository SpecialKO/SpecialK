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

#ifndef ULTRAHDR_DSP_ARM_MEM_NEON_H
#define ULTRAHDR_DSP_ARM_MEM_NEON_H

#include <arm_neon.h>

#include "ultrahdr/ultrahdrcommon.h"

namespace ultrahdr {

// The multi-vector load/store intrinsics are well-supported on AArch64 but
// only supported from GCC 14.1 (and not at all on Clang) for 32-bit platforms.
#if __aarch64__ || (!__clang__ && __GNUC__ >= 14)
#define COMPILER_SUPPORTS_LDST_MULTIPLE 1
#endif

static FORCE_INLINE uint8x16x2_t load_u8x16_x2(const uint8_t *src) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  return vld1q_u8_x2(src);
#else
  uint8x16x2_t res = {{vld1q_u8(src + 0), vld1q_u8(src + 16)}};
  return res;
#endif
}

static FORCE_INLINE uint8x16x4_t load_u8x16_x4(const uint8_t *src) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  return vld1q_u8_x4(src);
#else
  uint8x16x4_t res = {
      {vld1q_u8(src + 0), vld1q_u8(src + 16), vld1q_u8(src + 32), vld1q_u8(src + 48)}};
  return res;
#endif
}

static FORCE_INLINE uint16x8x2_t load_u16x8_x2(const uint16_t *src) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  return vld1q_u16_x2(src);
#else
  uint16x8x2_t res = {{vld1q_u16(src + 0), vld1q_u16(src + 8)}};
  return res;
#endif
}

static FORCE_INLINE uint16x8x4_t load_u16x8_x4(const uint16_t *src) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  return vld1q_u16_x4(src);
#else
  uint16x8x4_t res = {
      {vld1q_u16(src + 0), vld1q_u16(src + 8), vld1q_u16(src + 16), vld1q_u16(src + 24)}};
  return res;
#endif
}

static FORCE_INLINE uint32x4x2_t load_u32x4_x2(const uint32_t *src) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  return vld1q_u32_x2(src);
#else
  uint32x4x2_t res = {{vld1q_u32(src + 0), vld1q_u32(src + 4)}};
  return res;
#endif
}

static FORCE_INLINE uint32x4x4_t load_u32x4_x4(const uint32_t *src) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  return vld1q_u32_x4(src);
#else
  uint32x4x4_t res = {
      {vld1q_u32(src + 0), vld1q_u32(src + 4), vld1q_u32(src + 8), vld1q_u32(src + 12)}};
  return res;
#endif
}

static FORCE_INLINE void store_u8x16_x2(uint8_t *dst, uint8x16x2_t a) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  vst1q_u8_x2(dst, a);
#else
  vst1q_u8(dst + 0, a.val[0]);
  vst1q_u8(dst + 16, a.val[1]);
#endif
}

static FORCE_INLINE void store_u8x16_x4(uint8_t *dst, uint8x16x4_t a) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  vst1q_u8_x4(dst, a);
#else
  vst1q_u8(dst + 0, a.val[0]);
  vst1q_u8(dst + 16, a.val[1]);
  vst1q_u8(dst + 32, a.val[2]);
  vst1q_u8(dst + 48, a.val[3]);
#endif
}

static FORCE_INLINE void store_u16x8_x2(uint16_t *dst, uint16x8x2_t a) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  vst1q_u16_x2(dst, a);
#else
  vst1q_u16(dst + 0, a.val[0]);
  vst1q_u16(dst + 8, a.val[1]);
#endif
}

static FORCE_INLINE void store_u16x8_x4(uint16_t *dst, uint16x8x4_t a) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  vst1q_u16_x4(dst, a);
#else
  vst1q_u16(dst + 0, a.val[0]);
  vst1q_u16(dst + 8, a.val[1]);
  vst1q_u16(dst + 16, a.val[2]);
  vst1q_u16(dst + 24, a.val[3]);
#endif
}

static FORCE_INLINE void store_u32x4_x2(uint32_t *dst, uint32x4x2_t a) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  vst1q_u32_x2(dst, a);
#else
  vst1q_u32(dst + 0, a.val[0]);
  vst1q_u32(dst + 4, a.val[1]);
#endif
}

static FORCE_INLINE void store_u32x4_x4(uint32_t *dst, uint32x4x4_t a) {
#ifdef COMPILER_SUPPORTS_LDST_MULTIPLE
  vst1q_u32_x4(dst, a);
#else
  vst1q_u32(dst + 0, a.val[0]);
  vst1q_u32(dst + 4, a.val[1]);
  vst1q_u32(dst + 8, a.val[2]);
  vst1q_u32(dst + 12, a.val[3]);
#endif
}

}  // namespace ultrahdr

#endif  // ULTRAHDR_DSP_ARM_MEM_NEON_H
