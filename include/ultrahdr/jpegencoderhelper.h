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

#ifndef ULTRAHDR_JPEGENCODERHELPER_H
#define ULTRAHDR_JPEGENCODERHELPER_H

#include <stdio.h>  // For jpeglib.h.

// C++ build requires extern C for jpeg internals.
#ifdef __cplusplus
extern "C" {
#endif

#include <jerror.h>
#include <jpeglib.h>

#ifdef __cplusplus
}  // extern "C"
#endif

#include <cstdint>
#include <vector>

#include "ultrahdr_api.h"

namespace ultrahdr {

/*!\brief module for managing output */
struct destination_mgr_impl : jpeg_destination_mgr {
  static const int kBlockSize = 16384;  // result buffer resize step
  std::vector<JOCTET> mResultBuffer;    // buffer to store encoded data
};

/*!\brief Encapsulates a converter from raw to jpg image format. This class is not thread-safe */
class JpegEncoderHelper {
 public:
  JpegEncoderHelper() = default;
  ~JpegEncoderHelper() = default;

  /*!\brief This function encodes the raw image that is passed to it and stores the results
   * internally. The result is accessible via getter functions.
   *
   * \param[in]  img        image to encode
   * \param[in]  qfactor    quality factor [1 - 100, 1 being poorest and 100 being best quality]
   * \param[in]  iccBuffer  pointer to icc segment that needs to be added to the compressed image
   * \param[in]  iccSize    size of icc segment
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t compressImage(const uhdr_raw_image_t* img, const int qfactor,
                                  const void* iccBuffer, const unsigned int iccSize);

  /*!\brief This function encodes the raw image that is passed to it and stores the results
   * internally. The result is accessible via getter functions.
   *
   * \param[in]  planes     pointers of all planes of input image
   * \param[in]  strides    strides of all planes of input image
   * \param[in]  width      image width
   * \param[in]  height     image height
   * \param[in]  format     input raw image format
   * \param[in]  qfactor    quality factor [1 - 100, 1 being poorest and 100 being best quality]
   * \param[in]  iccBuffer  pointer to icc segment that needs to be added to the compressed image
   * \param[in]  iccSize    size of icc segment
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t compressImage(const uint8_t* planes[3], const size_t strides[3],
                                  const int width, const int height, const uhdr_img_fmt_t format,
                                  const int qfactor, const void* iccBuffer,
                                  const unsigned int iccSize);

  /*! Below public methods are only effective if a call to compressImage() is made and it returned
   * true. */

  /*!\brief returns pointer to compressed image output */
  uhdr_compressed_image_t getCompressedImage();

  /*!\brief returns pointer to compressed image output
   * \deprecated This function is deprecated instead use getCompressedImage().
   */
  void* getCompressedImagePtr() { return mDestMgr.mResultBuffer.data(); }

  /*!\brief returns size of compressed image
   * \deprecated This function is deprecated instead use getCompressedImage().
   */
  size_t getCompressedImageSize() { return mDestMgr.mResultBuffer.size(); }

 private:
  // max number of components supported
  static constexpr int kMaxNumComponents = 3;

  uhdr_error_info_t encode(const uint8_t* planes[3], const size_t strides[3], const int width,
                           const int height, const uhdr_img_fmt_t format, const int qfactor,
                           const void* iccBuffer, const unsigned int iccSize);

  uhdr_error_info_t compressYCbCr(jpeg_compress_struct* cinfo, const uint8_t* planes[3],
                                  const size_t strides[3]);

  destination_mgr_impl mDestMgr;  // object for managing output

  // temporary storage
  std::unique_ptr<uint8_t[]> mPlanesMCURow[kMaxNumComponents];

  size_t mPlaneWidth[kMaxNumComponents];
  size_t mPlaneHeight[kMaxNumComponents];
};

} /* namespace ultrahdr  */

#endif  // ULTRAHDR_JPEGENCODERHELPER_H
