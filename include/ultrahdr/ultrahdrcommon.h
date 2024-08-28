/*
 * Copyright 2023 The Android Open Source Project
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

#ifndef ULTRAHDR_ULTRAHDRCOMMON_H
#define ULTRAHDR_ULTRAHDRCOMMON_H

//#define LOG_NDEBUG 0

#ifdef UHDR_ENABLE_GLES
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#endif

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ultrahdr_api.h"

// ===============================================================================================
// Function Macros
// ===============================================================================================

#ifdef __ANDROID__

#ifdef LOG_NDEBUG
#include "android/log.h"

#ifndef LOG_TAG
#define LOG_TAG "UHDR"
#endif

#ifndef ALOGD
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#endif

#ifndef ALOGE
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif

#ifndef ALOGI
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#endif

#ifndef ALOGV
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#endif

#ifndef ALOGW
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#endif

#else

#define ALOGD(...) ((void)0)
#define ALOGE(...) ((void)0)
#define ALOGI(...) ((void)0)
#define ALOGV(...) ((void)0)
#define ALOGW(...) ((void)0)

#endif

#else

#ifdef LOG_NDEBUG
#include <cstdio>

#define ALOGD(...)                \
  do {                            \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n");        \
  } while (0)

#define ALOGE(...)                \
  do {                            \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n");        \
  } while (0)

#define ALOGI(...)                \
  do {                            \
    fprintf(stdout, __VA_ARGS__); \
    fprintf(stdout, "\n");        \
  } while (0)

#define ALOGV(...)                \
  do {                            \
    fprintf(stdout, __VA_ARGS__); \
    fprintf(stdout, "\n");        \
  } while (0)

#define ALOGW(...)                \
  do {                            \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n");        \
  } while (0)

#else

#define ALOGD(...) ((void)0)
#define ALOGE(...) ((void)0)
#define ALOGI(...) ((void)0)
#define ALOGV(...) ((void)0)
#define ALOGW(...) ((void)0)

#endif

#endif

#define ALIGNM(x, m) ((((x) + ((m)-1)) / (m)) * (m))

#define UHDR_ERR_CHECK(x)                     \
  {                                           \
    uhdr_error_info_t status = (x);           \
    if (status.error_code != UHDR_CODEC_OK) { \
      return status;                          \
    }                                         \
  }

#if defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#define INLINE __inline
#else
#define FORCE_INLINE __inline__ __attribute__((always_inline))
#define INLINE inline
#endif

static const uhdr_error_info_t g_no_error = {UHDR_CODEC_OK, 0, ""};

namespace ultrahdr {

// ===============================================================================================
// Globals
// ===============================================================================================
extern const int kMinWidth, kMinHeight;
extern const int kMaxWidth, kMaxHeight;

// ===============================================================================================
// Structure Definitions
// ===============================================================================================

/**\brief uhdr memory block */
typedef struct uhdr_memory_block {
  uhdr_memory_block(size_t capacity);

  std::unique_ptr<uint8_t[]> m_buffer; /**< data */
  size_t m_capacity;                   /**< capacity */
} uhdr_memory_block_t;                 /**< alias for struct uhdr_memory_block */

/**\brief extended raw image descriptor */
typedef struct uhdr_raw_image_ext : uhdr_raw_image_t {
  uhdr_raw_image_ext(uhdr_img_fmt_t fmt, uhdr_color_gamut_t cg, uhdr_color_transfer_t ct,
                     uhdr_color_range_t range, unsigned w, unsigned h, unsigned align_stride_to);

 private:
  std::unique_ptr<ultrahdr::uhdr_memory_block> m_block;
} uhdr_raw_image_ext_t; /**< alias for struct uhdr_raw_image_ext */

/**\brief extended compressed image descriptor */
typedef struct uhdr_compressed_image_ext : uhdr_compressed_image_t {
  uhdr_compressed_image_ext(uhdr_color_gamut_t cg, uhdr_color_transfer_t ct,
                            uhdr_color_range_t range, unsigned sz);

 private:
  std::unique_ptr<ultrahdr::uhdr_memory_block> m_block;
} uhdr_compressed_image_ext_t; /**< alias for struct uhdr_compressed_image_ext */

/*!\brief forward declaration for image effect descriptor */
typedef struct uhdr_effect_desc uhdr_effect_desc_t;

/**\brief Gain map metadata. */
typedef struct uhdr_gainmap_metadata_ext : uhdr_gainmap_metadata {
  uhdr_gainmap_metadata_ext() {}

  uhdr_gainmap_metadata_ext(std::string ver) { version = ver; }

  uhdr_gainmap_metadata_ext(uhdr_gainmap_metadata& metadata, std::string ver) {
    max_content_boost = metadata.max_content_boost;
    min_content_boost = metadata.min_content_boost;
    gamma = metadata.gamma;
    offset_sdr = metadata.offset_sdr;
    offset_hdr = metadata.offset_hdr;
    hdr_capacity_min = metadata.hdr_capacity_min;
    hdr_capacity_max = metadata.hdr_capacity_max;
    version = ver;
  }

  std::string version;         /**< Ultra HDR format version */
} uhdr_gainmap_metadata_ext_t; /**< alias for struct uhdr_gainmap_metadata */

#ifdef UHDR_ENABLE_GLES

typedef enum uhdr_effect_shader {
  UHDR_MIR_HORZ,
  UHDR_MIR_VERT,
  UHDR_ROT_90,
  UHDR_ROT_180,
  UHDR_ROT_270,
  UHDR_CROP,
  UHDR_RESIZE,
} uhdr_effect_shader_t;

/**\brief OpenGL context */
typedef struct uhdr_opengl_ctxt {
  // EGL Context
  EGLDisplay mEGLDisplay; /**< EGL display connection */
  EGLContext mEGLContext; /**< EGL rendering context */
  EGLSurface mEGLSurface; /**< EGL surface for rendering */
  EGLConfig mEGLConfig;   /**< EGL frame buffer configuration */

  // GLES Context
  GLuint mQuadVAO, mQuadVBO, mQuadEBO;    /**< GL objects */
  GLuint mShaderProgram[UHDR_RESIZE + 1]; /**< Shader programs */
  uhdr_error_info_t mErrorStatus;         /**< Context status */

  uhdr_opengl_ctxt();
  ~uhdr_opengl_ctxt();

  /*!\brief Initializes the OpenGL context. Mainly it prepares EGL. We want a GLES3.0 context and a
   * surface that supports pbuffer. Once this is done and surface is made current, the gl state is
   * initialized
   *
   * \return none
   */
  void init_opengl_ctxt();

  /*!\brief This method is used to compile a shader
   *
   * \param[in]   type    shader type
   * \param[in]   source  shader source code
   *
   * \return GLuint #shader_id if operation succeeds, 0 otherwise.
   */
  GLuint compile_shader(GLenum type, const char* source);

  /*!\brief This method is used to create a shader program
   *
   * \param[in]   vertex_source      vertex shader source code
   * \param[in]   fragment_source    fragment shader source code
   *
   * \return GLuint #shader_program_id if operation succeeds, 0 otherwise.
   */
  GLuint create_shader_program(const char* vertex_source, const char* fragment_source);

  /*!\brief This method is used to create a 2D texture for a raw image
   * NOTE: For multichannel planar image, this method assumes the channel data to be contiguous
   * NOTE: For any channel, this method assumes width and stride to be identical
   *
   * \param[in]   fmt       image format
   * \param[in]   w         image width
   * \param[in]   h         image height
   * \param[in]   data      image data
   *
   * \return GLuint #texture_id if operation succeeds, 0 otherwise.
   */
  GLuint create_texture(uhdr_img_fmt_t fmt, int w, int h, void* data);

  /*!\breif This method is used to read data from texture into a raw image
   * NOTE: For any channel, this method assumes width and stride to be identical
   *
   * \param[in]   texture    texture_id
   * \param[in]   fmt        image format
   * \param[in]   w          image width
   * \param[in]   h          image height
   * \param[in]   data       image data
   *
   * \return none
   */
  void read_texture(GLuint* texture, uhdr_img_fmt_t fmt, int w, int h, void* data);

  /*!\brief This method is used to set up quad buffers and arrays
   *
   * \return none
   */
  void setup_quad();

  /*!\brief This method is used to set up frame buffer for a 2D texture
   *
   * \param[in]   texture         texture id
   *
   * \return GLuint #framebuffer_id if operation succeeds, 0 otherwise.
   */
  GLuint setup_framebuffer(GLuint& texture);

  /*!\brief Checks for gl errors. On error, internal error state is updated with details
   *
   * \param[in]   msg     useful description for logging
   *
   * \return none
   */
  void check_gl_errors(const char* msg);

  /*!\brief Reset the current context to default state for reuse
   *
   * \return none
   */
  void reset_opengl_ctxt();

  /*!\brief Deletes the current context
   *
   * \return none
   */
  void delete_opengl_ctxt();

} uhdr_opengl_ctxt_t; /**< alias for struct uhdr_opengl_ctxt */

bool isBufferDataContiguous(uhdr_raw_image_t* img);

#endif

}  // namespace ultrahdr

// ===============================================================================================
// Extensions of ultrahdr api definitions, so outside ultrahdr namespace
// ===============================================================================================

struct uhdr_codec_private {
  std::deque<ultrahdr::uhdr_effect_desc_t*> m_effects;
#ifdef UHDR_ENABLE_GLES
  ultrahdr::uhdr_opengl_ctxt_t m_uhdr_gl_ctxt;
  bool m_enable_gles;
#endif
  bool m_sailed;

  virtual ~uhdr_codec_private();
};

struct uhdr_encoder_private : uhdr_codec_private {
  // config data
  std::map<uhdr_img_label, std::unique_ptr<ultrahdr::uhdr_raw_image_ext_t>> m_raw_images;
  std::map<uhdr_img_label, std::unique_ptr<ultrahdr::uhdr_compressed_image_ext_t>>
      m_compressed_images;
  std::map<uhdr_img_label, int> m_quality;
  std::vector<uint8_t> m_exif;
  uhdr_gainmap_metadata_t m_metadata;
  uhdr_codec_t m_output_format;
  int m_gainmap_scale_factor;
  bool m_use_multi_channel_gainmap;
  float m_gamma;
  uhdr_enc_preset_t m_enc_preset;
  float m_min_content_boost;
  float m_max_content_boost;

  // internal data
  std::unique_ptr<ultrahdr::uhdr_compressed_image_ext_t> m_compressed_output_buffer;
  uhdr_error_info_t m_encode_call_status;
};

struct uhdr_decoder_private : uhdr_codec_private {
  // config data
  std::unique_ptr<ultrahdr::uhdr_compressed_image_ext_t> m_uhdr_compressed_img;
  uhdr_img_fmt_t m_output_fmt;
  uhdr_color_transfer_t m_output_ct;
  float m_output_max_disp_boost;

  // internal data
  bool m_probed;
  std::unique_ptr<ultrahdr::uhdr_raw_image_ext_t> m_decoded_img_buffer;
  std::unique_ptr<ultrahdr::uhdr_raw_image_ext_t> m_gainmap_img_buffer;
  int m_img_wd, m_img_ht;
  int m_gainmap_wd, m_gainmap_ht, m_gainmap_num_comp;
  std::vector<uint8_t> m_exif;
  uhdr_mem_block_t m_exif_block;
  std::vector<uint8_t> m_icc;
  uhdr_mem_block_t m_icc_block;
  uhdr_gainmap_metadata_t m_metadata;
  uhdr_error_info_t m_probe_call_status;
  uhdr_error_info_t m_decode_call_status;
};

#endif  // ULTRAHDR_ULTRAHDRCOMMON_H
