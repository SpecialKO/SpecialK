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

#ifndef ULTRAHDR_JPEGDECODERHELPER_H
#define ULTRAHDR_JPEGDECODERHELPER_H

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
#include <memory>
#include <vector>

#include "ultrahdr_api.h"

namespace ultrahdr {

/*!\brief List of supported operations */
typedef enum {
  PARSE_STREAM = (1 << 0),   /**< Parse jpeg header, APPn markers (Exif, Icc, Xmp, Iso) */
  DECODE_STREAM = (1 << 16), /**< Single channel images are decoded to Grayscale format and multi
                                channel images are decoded to RGB format */
  DECODE_TO_YCBCR_CS = (1 << 17), /**< Decode image to YCbCr Color Space  */
  DECODE_TO_RGB_CS = (1 << 18),   /**< Decode image to RGB Color Space  */
} decode_mode_t;

/*!\brief Encapsulates a converter from JPEG to raw image format. This class is not thread-safe */
class JpegDecoderHelper {
 public:
  JpegDecoderHelper() = default;
  ~JpegDecoderHelper() = default;

  /*!\brief This function decodes the bitstream that is passed to it to the desired format and
   * stores the results internally. The result is accessible via getter functions.
   *
   * \param[in]  image    pointer to compressed image
   * \param[in]  length   length of compressed image
   * \param[in]  mode     output decode format
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t decompressImage(const void* image, int length,
                                    decode_mode_t mode = DECODE_TO_YCBCR_CS);

  /*!\brief This function parses the bitstream that is passed to it and makes image information
   * available to the client via getter() functions. It does not decompress the image. That is done
   * by decompressImage().
   *
   * \param[in]  image    pointer to compressed image
   * \param[in]  length   length of compressed image
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t parseImage(const void* image, int length) {
    return decompressImage(image, length, PARSE_STREAM);
  }

  /*! Below public methods are only effective if a call to decompressImage() is made and it returned
   * true. */

  /*!\brief returns decompressed image descriptor */
  uhdr_raw_image_t getDecompressedImage();

  /*!\brief returns pointer to decompressed image
   * \deprecated This function is deprecated instead use getDecompressedImage().
   */
  void* getDecompressedImagePtr() { return mResultBuffer.data(); }

  /*!\brief returns size of decompressed image
   * \deprecated This function is deprecated instead use getDecompressedImage().
   */
  size_t getDecompressedImageSize() { return mResultBuffer.size(); }

  /*! Below public methods are only effective if a call to parseImage() or decompressImage() is made
   * and it returned true. */

  /*!\brief returns image width */
  size_t getDecompressedImageWidth() { return mPlaneWidth[0]; }

  /*!\brief returns image height */
  size_t getDecompressedImageHeight() { return mPlaneHeight[0]; }

  /*!\brief returns number of components in image */
  size_t getNumComponentsInImage() { return mNumComponents; }

  /*!\brief returns pointer to xmp block present in input image */
  void* getXMPPtr() { return mXMPBuffer.data(); }

  /*!\brief returns size of xmp block */
  size_t getXMPSize() { return mXMPBuffer.size(); }

  /*!\brief returns pointer to exif block present in input image */
  void* getEXIFPtr() { return mEXIFBuffer.data(); }

  /*!\brief returns size of exif block */
  size_t getEXIFSize() { return mEXIFBuffer.size(); }

  /*!\brief returns pointer to icc block present in input image */
  void* getICCPtr() { return mICCBuffer.data(); }

  /*!\brief returns size of icc block */
  size_t getICCSize() { return mICCBuffer.size(); }

  /*!\brief returns pointer to iso block present in input image */
  void* getIsoMetadataPtr() { return mIsoMetadataBuffer.data(); }

  /*!\brief returns size of iso block */
  size_t getIsoMetadataSize() { return mIsoMetadataBuffer.size(); }

  /*!\brief returns the offset of exif data payload with reference to 'image' address that is passed
   * via parseImage()/decompressImage() call. Note this does not include jpeg marker (0xffe1) and
   * the next 2 bytes indicating the size of the payload. If exif block is not present in the image
   * passed, then it returns -1. */
  int getEXIFPos() { return mExifPayLoadOffset; }

 private:
  // max number of components supported
  static constexpr int kMaxNumComponents = 3;

  uhdr_error_info_t decode(const void* image, int length, decode_mode_t mode);
  uhdr_error_info_t decode(jpeg_decompress_struct* cinfo, uint8_t* dest);
  uhdr_error_info_t decodeToCSYCbCr(jpeg_decompress_struct* cinfo, uint8_t* dest);
  uhdr_error_info_t decodeToCSRGB(jpeg_decompress_struct* cinfo, uint8_t* dest);

  // temporary storage
  std::unique_ptr<uint8_t[]> mPlanesMCURow[kMaxNumComponents];

  std::vector<JOCTET> mResultBuffer;       // buffer to store decoded data
  std::vector<JOCTET> mXMPBuffer;          // buffer to store xmp data
  std::vector<JOCTET> mEXIFBuffer;         // buffer to store exif data
  std::vector<JOCTET> mICCBuffer;          // buffer to store icc data
  std::vector<JOCTET> mIsoMetadataBuffer;  // buffer to store iso data

  // image attributes
  uhdr_img_fmt_t mOutFormat;
  size_t mNumComponents;
  size_t mPlaneWidth[kMaxNumComponents];
  size_t mPlaneHeight[kMaxNumComponents];
  size_t mPlaneHStride[kMaxNumComponents];
  size_t mPlaneVStride[kMaxNumComponents];

  int mExifPayLoadOffset;  // Position of EXIF package, default value is -1 which means no EXIF
                           // package appears.
};

} /* namespace ultrahdr  */

#endif  // ULTRAHDR_JPEGDECODERHELPER_H
