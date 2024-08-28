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

#ifndef ULTRAHDR_EDITORHELPER_H
#define ULTRAHDR_EDITORHELPER_H

#include "ultrahdr_api.h"
#include "ultrahdr/ultrahdrcommon.h"

namespace ultrahdr {

/*!\brief uhdr image effect descriptor */
typedef struct uhdr_effect_desc {
  virtual std::string to_string() = 0;

  virtual ~uhdr_effect_desc() = default;
} uhdr_effect_desc_t; /**< alias for struct uhdr_effect_desc */

/*!\brief mirror effect descriptor */
typedef struct uhdr_mirror_effect : uhdr_effect_desc {
  uhdr_mirror_effect(uhdr_mirror_direction_t direction);

  std::string to_string() {
    return "effect : mirror, metadata : direction - " + ((m_direction == UHDR_MIRROR_HORIZONTAL)
                                                             ? std::string{"horizontal"}
                                                             : std::string{"vertical"});
  }

  uhdr_mirror_direction_t m_direction;

  void (*m_mirror_uint8_t)(uint8_t*, uint8_t*, int, int, int, int, uhdr_mirror_direction_t);
  void (*m_mirror_uint16_t)(uint16_t*, uint16_t*, int, int, int, int, uhdr_mirror_direction_t);
  void (*m_mirror_uint32_t)(uint32_t*, uint32_t*, int, int, int, int, uhdr_mirror_direction_t);
  void (*m_mirror_uint64_t)(uint64_t*, uint64_t*, int, int, int, int, uhdr_mirror_direction_t);
} uhdr_mirror_effect_t; /**< alias for struct uhdr_mirror_effect */

/*!\brief rotate effect descriptor */
typedef struct uhdr_rotate_effect : uhdr_effect_desc {
  uhdr_rotate_effect(int degree);

  std::string to_string() {
    return "effect : rotate, metadata : degree - " + std::to_string(m_degree);
  }

  int m_degree;

  void (*m_rotate_uint8_t)(uint8_t*, uint8_t*, int, int, int, int, int);
  void (*m_rotate_uint16_t)(uint16_t*, uint16_t*, int, int, int, int, int);
  void (*m_rotate_uint32_t)(uint32_t*, uint32_t*, int, int, int, int, int);
  void (*m_rotate_uint64_t)(uint64_t*, uint64_t*, int, int, int, int, int);
} uhdr_rotate_effect_t; /**< alias for struct uhdr_rotate_effect */

/*!\brief crop effect descriptor */
typedef struct uhdr_crop_effect : uhdr_effect_desc {
  uhdr_crop_effect(int left, int right, int top, int bottom)
      : m_left{left}, m_right{right}, m_top{top}, m_bottom{bottom} {}

  std::string to_string() {
    return "effect : crop, metadata : left, right, top, bottom - " + std::to_string(m_left) + " ," +
           std::to_string(m_right) + " ," + std::to_string(m_top) + " ," + std::to_string(m_bottom);
  }

  int m_left;
  int m_right;
  int m_top;
  int m_bottom;
} uhdr_crop_effect_t; /**< alias for struct uhdr_crop_effect */

/*!\brief resize effect descriptor */
typedef struct uhdr_resize_effect : uhdr_effect_desc {
  uhdr_resize_effect(int width, int height);

  std::string to_string() {
    return "effect : resize, metadata : dimensions w, h" + std::to_string(m_width) + " ," +
           std::to_string(m_height);
  }

  int m_width;
  int m_height;

  void (*m_resize_uint8_t)(uint8_t*, uint8_t*, int, int, int, int, int, int);
  void (*m_resize_uint16_t)(uint16_t*, uint16_t*, int, int, int, int, int, int);
  void (*m_resize_uint32_t)(uint32_t*, uint32_t*, int, int, int, int, int, int);
  void (*m_resize_uint64_t)(uint64_t*, uint64_t*, int, int, int, int, int, int);
} uhdr_resize_effect_t; /**< alias for struct uhdr_resize_effect */

template <typename T>
extern void rotate_buffer_clockwise(T* src_buffer, T* dst_buffer, int src_w, int src_h,
                                    int src_stride, int dst_stride, int degree);

template <typename T>
extern void mirror_buffer(T* src_buffer, T* dst_buffer, int src_w, int src_h, int src_stride,
                          int dst_stride, uhdr_mirror_direction_t direction);

template <typename T>
extern void resize_buffer(T* src_buffer, T* dst_buffer, int src_w, int src_h, int dst_w, int dst_h,
                          int src_stride, int dst_stride);

#if (defined(UHDR_ENABLE_INTRINSICS) && (defined(__ARM_NEON__) || defined(__ARM_NEON)))
template <typename T>
extern void mirror_buffer_neon(T* src_buffer, T* dst_buffer, int src_w, int src_h, int src_stride,
                               int dst_stride, uhdr_mirror_direction_t direction);

template <typename T>
extern void rotate_buffer_clockwise_neon(T* src_buffer, T* dst_buffer, int src_w, int src_h,
                                         int src_stride, int dst_stride, int degrees);
#endif

#ifdef UHDR_ENABLE_GLES

std::unique_ptr<uhdr_raw_image_ext_t> apply_resize_gles(uhdr_raw_image_t* src, int dst_w, int dst_h,
                                                        uhdr_opengl_ctxt* gl_ctxt,
                                                        GLuint* srcTexture);

std::unique_ptr<uhdr_raw_image_ext_t> apply_mirror_gles(ultrahdr::uhdr_mirror_effect_t* desc,
                                                        uhdr_raw_image_t* src,
                                                        uhdr_opengl_ctxt* gl_ctxt,
                                                        GLuint* srcTexture);

std::unique_ptr<uhdr_raw_image_ext_t> apply_rotate_gles(ultrahdr::uhdr_rotate_effect_t* desc,
                                                        uhdr_raw_image_t* src,
                                                        uhdr_opengl_ctxt* gl_ctxt,
                                                        GLuint* srcTexture);

void apply_crop_gles(uhdr_raw_image_t* src, int left, int top, int wd, int ht,
                     uhdr_opengl_ctxt* gl_ctxt, GLuint* srcTexture);
#endif

std::unique_ptr<uhdr_raw_image_ext_t> apply_rotate(ultrahdr::uhdr_rotate_effect_t* desc,
                                                   uhdr_raw_image_t* src, void* gl_ctxt = nullptr,
                                                   void* texture = nullptr);

std::unique_ptr<uhdr_raw_image_ext_t> apply_mirror(ultrahdr::uhdr_mirror_effect_t* desc,
                                                   uhdr_raw_image_t* src, void* gl_ctxt = nullptr,
                                                   void* texture = nullptr);

std::unique_ptr<uhdr_raw_image_ext_t> apply_resize(ultrahdr::uhdr_resize_effect_t* desc,
                                                   uhdr_raw_image* src, int dst_w, int dst_h,
                                                   void* gl_ctxt = nullptr,
                                                   void* texture = nullptr);

void apply_crop(uhdr_raw_image_t* src, int left, int top, int wd, int ht, void* gl_ctxt = nullptr,
                void* texture = nullptr);

}  // namespace ultrahdr

#endif  // ULTRAHDR_EDITORHELPER_H
