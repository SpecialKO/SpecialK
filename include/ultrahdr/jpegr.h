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

#ifndef ULTRAHDR_JPEGR_H
#define ULTRAHDR_JPEGR_H

#include <array>
#include <cfloat>

#include "ultrahdr_api.h"
#include "ultrahdr/ultrahdr.h"
#include "ultrahdr/ultrahdrcommon.h"
#include "ultrahdr/jpegdecoderhelper.h"
#include "ultrahdr/jpegencoderhelper.h"

namespace ultrahdr {

// Default configurations
// Map is quarter res / sixteenth size
static const size_t kMapDimensionScaleFactorDefault = 4;

// JPEG compress quality (0 ~ 100) for gain map
static const int kMapCompressQualityDefault = 85;

// Gain map calculation
static const bool kUseMultiChannelGainMapDefault = false;
// Default gamma value for gain map
static const float kGainMapGammaDefault = 1.0f;

// The current JPEGR version that we encode to
static const char* const kGainMapVersion = "1.0";
static const char* const kJpegrVersion = "1.0";

/*
 * Holds information of jpeg image
 */
struct jpeg_info_struct {
  std::vector<uint8_t> imgData = std::vector<uint8_t>(0);
  std::vector<uint8_t> iccData = std::vector<uint8_t>(0);
  std::vector<uint8_t> exifData = std::vector<uint8_t>(0);
  std::vector<uint8_t> xmpData = std::vector<uint8_t>(0);
  std::vector<uint8_t> isoData = std::vector<uint8_t>(0);
  size_t width;
  size_t height;
  size_t numComponents;
};

/*
 * Holds information of jpegr image
 */
struct jpegr_info_struct {
  size_t width;   // copy of primary image width (for easier access)
  size_t height;  // copy of primary image height (for easier access)
  jpeg_info_struct* primaryImgInfo = nullptr;
  jpeg_info_struct* gainmapImgInfo = nullptr;
};

typedef struct jpeg_info_struct* j_info_ptr;
typedef struct jpegr_info_struct* jr_info_ptr;

class JpegR {
 public:
  JpegR(void* uhdrGLESCtxt = nullptr,
        size_t mapDimensionScaleFactor = kMapDimensionScaleFactorDefault,
        int mapCompressQuality = kMapCompressQualityDefault,
        bool useMultiChannelGainMap = kUseMultiChannelGainMapDefault,
        float gamma = kGainMapGammaDefault, uhdr_enc_preset_t preset = UHDR_USAGE_REALTIME,
        float minContentBoost = FLT_MIN, float maxContentBoost = FLT_MAX);

  /*!\brief Encode API-0.
   *
   * Create ultrahdr jpeg image from raw hdr intent.
   *
   * Experimental only.
   *
   * Input hdr image is tonemapped to sdr image. A gainmap coefficient is computed between hdr and
   * sdr intent. sdr intent and gain map coefficient are compressed using jpeg encoding. compressed
   * gainmap is appended at the end of compressed sdr image.
   *
   * \param[in]       hdr_intent        hdr intent raw input image descriptor
   * \param[in, out]  dest              output image descriptor to store compressed ultrahdr image
   * \param[in]       quality           quality factor for sdr intent jpeg compression
   * \param[in]       exif              optional exif metadata that needs to be inserted in
   *                                    compressed output
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t encodeJPEGR(uhdr_raw_image_t* hdr_intent, uhdr_compressed_image_t* dest,
                                int quality, uhdr_mem_block_t* exif);

  /*!\brief Encode API-1.
   *
   * Create ultrahdr jpeg image from raw hdr intent and raw sdr intent.
   *
   * A gainmap coefficient is computed between hdr and sdr intent. sdr intent and gain map
   * coefficient are compressed using jpeg encoding. compressed gainmap is appended at the end of
   * compressed sdr image.
   * NOTE: Color transfer of sdr intent is expected to be sRGB.
   *
   * \param[in]       hdr_intent        hdr intent raw input image descriptor
   * \param[in]       sdr_intent        sdr intent raw input image descriptor
   * \param[in, out]  dest              output image descriptor to store compressed ultrahdr image
   * \param[in]       quality           quality factor for sdr intent jpeg compression
   * \param[in]       exif              optional exif metadata that needs to be inserted in
   *                                    compressed output
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t encodeJPEGR(uhdr_raw_image_t* hdr_intent, uhdr_raw_image_t* sdr_intent,
                                uhdr_compressed_image_t* dest, int quality, uhdr_mem_block_t* exif);

  /*!\brief Encode API-2.
   *
   * Create ultrahdr jpeg image from raw hdr intent, raw sdr intent and compressed sdr intent.
   *
   * A gainmap coefficient is computed between hdr and sdr intent. gain map coefficient is
   * compressed using jpeg encoding. compressed gainmap is appended at the end of compressed sdr
   * intent. ICC profile is added if one isn't present in the sdr intent JPEG image.
   * NOTE: Color transfer of sdr intent is expected to be sRGB.
   * NOTE: sdr intent raw and compressed inputs are expected to be related via compress/decompress
   * operations.
   *
   * \param[in]       hdr_intent               hdr intent raw input image descriptor
   * \param[in]       sdr_intent               sdr intent raw input image descriptor
   * \param[in]       sdr_intent_compressed    sdr intent compressed input image descriptor
   * \param[in, out]  dest                     output image descriptor to store compressed ultrahdr
   *                                           image
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t encodeJPEGR(uhdr_raw_image_t* hdr_intent, uhdr_raw_image_t* sdr_intent,
                                uhdr_compressed_image_t* sdr_intent_compressed,
                                uhdr_compressed_image_t* dest);

  /*!\brief Encode API-3.
   *
   * Create ultrahdr jpeg image from raw hdr intent and compressed sdr intent.
   *
   * The sdr intent is decoded and a gainmap coefficient is computed between hdr and sdr intent.
   * gain map coefficient is compressed using jpeg encoding. compressed gainmap is appended at the
   * end of compressed sdr image. ICC profile is added if one isn't present in the sdr intent JPEG
   * image.
   * NOTE: Color transfer of sdr intent is expected to be sRGB.
   *
   * \param[in]       hdr_intent               hdr intent raw input image descriptor
   * \param[in]       sdr_intent_compressed    sdr intent compressed input image descriptor
   * \param[in, out]  dest                     output image descriptor to store compressed ultrahdr
   *                                           image
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t encodeJPEGR(uhdr_raw_image_t* hdr_intent,
                                uhdr_compressed_image_t* sdr_intent_compressed,
                                uhdr_compressed_image_t* dest);

  /*!\brief Encode API-4.
   *
   * Create ultrahdr jpeg image from compressed sdr image and compressed gainmap image
   *
   * compressed gainmap image is added at the end of compressed sdr image. ICC profile is added if
   * one isn't present in the sdr intent compressed image.
   *
   * \param[in]       base_img_compressed      sdr intent compressed input image descriptor
   * \param[in]       gainmap_img_compressed   gainmap compressed image descriptor
   * \param[in]       metadata                 gainmap metadata descriptor
   * \param[in, out]  dest                     output image descriptor to store compressed ultrahdr
   *                                           image
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t encodeJPEGR(uhdr_compressed_image_t* base_img_compressed,
                                uhdr_compressed_image_t* gainmap_img_compressed,
                                uhdr_gainmap_metadata_ext_t* metadata,
                                uhdr_compressed_image_t* dest);

  /*!\brief Decode API.
   *
   * Decompress ultrahdr jpeg image.
   *
   * NOTE: This method requires that the ultrahdr input image contains an ICC profile with primaries
   * that match those of a color gamut that this library is aware of; Bt.709, Display-P3, or
   * Bt.2100. It also assumes the base image color transfer characteristics are sRGB.
   *
   * \param[in]       uhdr_compressed_img      compressed ultrahdr image descriptor
   * \param[in, out]  dest                     output image descriptor to store decoded output
   * \param[in]       max_display_boost        (optional) the maximum available boost supported by a
   *                                           display, the value must be greater than or equal
   *                                           to 1.0
   * \param[in]       output_ct                (optional) output color transfer
   * \param[in]       output_format            (optional) output pixel format
   * \param[in, out]  gainmap_img              (optional) output image descriptor to store decoded
   *                                           gainmap image
   * \param[in, out]  gainmap_metadata         (optional) descriptor to store gainmap metadata
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   *
   * NOTE: This method only supports single gain map metadata values for fields that allow
   * multi-channel metadata values.
   *
   * NOTE: Not all combinations of output color transfer and output pixel format are supported.
   * Refer below table for supported combinations.
   *         ----------------------------------------------------------------------
   *         |           color transfer	       |          color format            |
   *         ----------------------------------------------------------------------
   *         |                 SDR             |          32bppRGBA8888           |
   *         ----------------------------------------------------------------------
   *         |             HDR_LINEAR          |          64bppRGBAHalfFloat      |
   *         ----------------------------------------------------------------------
   *         |               HDR_PQ            |          32bppRGBA1010102        |
   *         ----------------------------------------------------------------------
   *         |               HDR_HLG           |          32bppRGBA1010102        |
   *         ----------------------------------------------------------------------
   */
  uhdr_error_info_t decodeJPEGR(uhdr_compressed_image_t* uhdr_compressed_img,
                                uhdr_raw_image_t* dest, float max_display_boost = FLT_MAX,
                                uhdr_color_transfer_t output_ct = UHDR_CT_LINEAR,
                                uhdr_img_fmt_t output_format = UHDR_IMG_FMT_64bppRGBAHalfFloat,
                                uhdr_raw_image_t* gainmap_img = nullptr,
                                uhdr_gainmap_metadata_t* gainmap_metadata = nullptr);

  /*!\brief This function parses the bitstream and returns information that is useful for actual
   * decoding. This does not decode the image. That is handled by decodeJPEGR
   *
   * \param[in]       uhdr_compressed_img      compressed ultrahdr image descriptor
   * \param[in, out]  uhdr_image_info          image info descriptor
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t getJPEGRInfo(uhdr_compressed_image_t* uhdr_compressed_img,
                                 jr_info_ptr uhdr_image_info);

  /*!\brief set gain map dimension scale factor
   * NOTE: Applicable only in encoding scenario
   *
   * \param[in]       mapDimensionScaleFactor      scale factor
   *
   * \return none
   */
  void setMapDimensionScaleFactor(size_t mapDimensionScaleFactor) {
    this->mMapDimensionScaleFactor = mapDimensionScaleFactor;
  }

  /*!\brief get gain map dimension scale factor
   * NOTE: Applicable only in encoding scenario
   *
   * \return mapDimensionScaleFactor
   */
  size_t getMapDimensionScaleFactor() { return this->mMapDimensionScaleFactor; }

  /*!\brief set gain map compression quality factor
   * NOTE: Applicable only in encoding scenario
   *
   * \param[in]       mapCompressQuality      quality factor for gain map image compression
   *
   * \return none
   */
  void setMapCompressQuality(int mapCompressQuality) {
    this->mMapCompressQuality = mapCompressQuality;
  }

  /*!\brief get gain map quality factor
   * NOTE: Applicable only in encoding scenario
   *
   * \return quality factor
   */
  int getMapCompressQuality() { return this->mMapCompressQuality; }

  /*!\brief set gain map gamma
   * NOTE: Applicable only in encoding scenario
   *
   * \param[in]       gamma      gamma parameter that is used for gain map calculation
   *
   * \return none
   */
  void setGainMapGamma(float gamma) { this->mGamma = gamma; }

  /*!\brief get gain map gamma
   * NOTE: Applicable only in encoding scenario
   *
   * \return gamma parameter
   */
  float getGainMapGamma() { return this->mGamma; }

  /*!\brief enable / disable multi channel gain map
   * NOTE: Applicable only in encoding scenario
   *
   * \param[in]       useMultiChannelGainMap      enable / disable multi channel gain map
   *
   * \return none
   */
  void setUseMultiChannelGainMap(bool useMultiChannelGainMap) {
    this->mUseMultiChannelGainMap = useMultiChannelGainMap;
  }

  /*!\brief check if multi channel gain map is enabled
   * NOTE: Applicable only in encoding scenario
   *
   * \return true if multi channel gain map is enabled, false otherwise
   */
  bool isUsingMultiChannelGainMap() { return this->mUseMultiChannelGainMap; }

  /*!\brief set gain map min and max content boost
   * NOTE: Applicable only in encoding scenario
   *
   * \param[in]       minBoost      gain map min content boost
   * \param[in]       maxBoost      gain map max content boost
   *
   * \return none
   */
  void setGainMapMinMaxContentBoost(float minBoost, float maxBoost) {
    this->mMinContentBoost = minBoost;
    this->mMaxContentBoost = maxBoost;
  }

  /*!\brief get gain map min max content boost
   * NOTE: Applicable only in encoding scenario
   *
   * \param[out]       minBoost      gain map min content boost
   * \param[out]       maxBoost      gain map max content boost
   *
   * \return none
   */
  void getGainMapMinMaxContentBoost(float& minBoost, float& maxBoost) {
    minBoost = this->mMinContentBoost;
    maxBoost = this->mMaxContentBoost;
  }

  /* \brief Alias of Encode API-0.
   *
   * \deprecated This function is deprecated. Use its alias
   */
  status_t encodeJPEGR(jr_uncompressed_ptr p010_image_ptr, ultrahdr_transfer_function hdr_tf,
                       jr_compressed_ptr dest, int quality, jr_exif_ptr exif);

  /* \brief Alias of Encode API-1.
   *
   * \deprecated This function is deprecated. Use its actual
   */
  status_t encodeJPEGR(jr_uncompressed_ptr p010_image_ptr, jr_uncompressed_ptr yuv420_image_ptr,
                       ultrahdr_transfer_function hdr_tf, jr_compressed_ptr dest, int quality,
                       jr_exif_ptr exif);

  /* \brief Alias of Encode API-2.
   *
   * \deprecated This function is deprecated. Use its actual
   */
  status_t encodeJPEGR(jr_uncompressed_ptr p010_image_ptr, jr_uncompressed_ptr yuv420_image_ptr,
                       jr_compressed_ptr yuv420jpg_image_ptr, ultrahdr_transfer_function hdr_tf,
                       jr_compressed_ptr dest);

  /* \brief Alias of Encode API-3.
   *
   * \deprecated This function is deprecated. Use its actual
   */
  status_t encodeJPEGR(jr_uncompressed_ptr p010_image_ptr, jr_compressed_ptr yuv420jpg_image_ptr,
                       ultrahdr_transfer_function hdr_tf, jr_compressed_ptr dest);

  /* \brief Alias of Encode API-4.
   *
   * \deprecated This function is deprecated. Use its actual
   */
  status_t encodeJPEGR(jr_compressed_ptr yuv420jpg_image_ptr,
                       jr_compressed_ptr gainmapjpg_image_ptr, ultrahdr_metadata_ptr metadata,
                       jr_compressed_ptr dest);

  /* \brief Alias of Decode API
   *
   * \deprecated This function is deprecated. Use its actual
   */
  status_t decodeJPEGR(jr_compressed_ptr jpegr_image_ptr, jr_uncompressed_ptr dest,
                       float max_display_boost = FLT_MAX, jr_exif_ptr exif = nullptr,
                       ultrahdr_output_format output_format = ULTRAHDR_OUTPUT_HDR_LINEAR,
                       jr_uncompressed_ptr gainmap_image_ptr = nullptr,
                       ultrahdr_metadata_ptr metadata = nullptr);

  /* \brief Alias of getJPEGRInfo
   *
   * \deprecated This function is deprecated. Use its actual
   */
  status_t getJPEGRInfo(jr_compressed_ptr jpegr_image_ptr, jr_info_ptr jpegr_image_info_ptr);

  /*!\brief This function receives iso block and / or xmp block and parses gainmap metadata and fill
   * the output descriptor. If both iso block and xmp block are available, then iso block is
   * preferred over xmp.
   *
   * \param[in]       iso_data                  iso memory block
   * \param[in]       iso_size                  iso block size
   * \param[in]       xmp_data                  xmp memory block
   * \param[in]       xmp_size                  xmp block size
   * \param[in, out]  gainmap_metadata          gainmap metadata descriptor
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t parseGainMapMetadata(uint8_t* iso_data, int iso_size, uint8_t* xmp_data,
                                         int xmp_size, uhdr_gainmap_metadata_ext_t* uhdr_metadata);

 protected:
  /*!\brief This method takes hdr intent and sdr intent and computes gainmap coefficient.
   *
   * This method is called in the encoding pipeline. It takes uncompressed 8-bit and 10-bit yuv
   * images as input and calculates gainmap.
   *
   * NOTE: The input images must be the same resolution.
   * NOTE: The SDR input is assumed to use the sRGB transfer function.
   *
   * \param[in]       sdr_intent               sdr intent raw input image descriptor
   * \param[in]       hdr_intent               hdr intent raw input image descriptor
   * \param[in, out]  gainmap_metadata         gainmap metadata descriptor
   * \param[in, out]  gainmap_img              gainmap image descriptor
   * \param[in]       sdr_is_601               (optional) if sdr_is_601 is true, then use BT.601
   *                                           gamut to represent sdr intent regardless of the value
   *                                           present in the sdr intent image descriptor
   * \param[in]       use_luminance            (optional) used for single channel gainmap. If
   *                                           use_luminance is true, gainmap calculation is based
   *                                           on the pixel's luminance which is a weighted
   *                                           combination of r, g, b channels; otherwise, gainmap
   *                                           calculation is based of the maximun value of r, g, b
   *                                           channels.
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t generateGainMap(uhdr_raw_image_t* sdr_intent, uhdr_raw_image_t* hdr_intent,
                                    uhdr_gainmap_metadata_ext_t* gainmap_metadata,
                                    std::unique_ptr<uhdr_raw_image_ext_t>& gainmap_img,
                                    bool sdr_is_601 = false, bool use_luminance = true);

  /*!\brief This method takes sdr intent, gainmap image and gainmap metadata and computes hdr
   * intent. This method is called in the decoding pipeline. The output hdr intent image will have
   * same color gamut as sdr intent.
   *
   * NOTE: The SDR input is assumed to use the sRGB transfer function.
   *
   * \param[in]       sdr_intent               sdr intent raw input image descriptor
   * \param[in]       gainmap_img              gainmap image descriptor
   * \param[in]       gainmap_metadata         gainmap metadata descriptor
   * \param[in]       output_ct                output color transfer
   * \param[in]       output_format            output pixel format
   * \param[in]       max_display_boost        the maximum available boost supported by a
   *                                           display, the value must be greater than or equal
   *                                           to 1.0
   * \param[in, out]  dest                     output image descriptor to store output
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t applyGainMap(uhdr_raw_image_t* sdr_intent, uhdr_raw_image_t* gainmap_img,
                                 uhdr_gainmap_metadata_ext_t* gainmap_metadata,
                                 uhdr_color_transfer_t output_ct, uhdr_img_fmt_t output_format,
                                 float max_display_boost, uhdr_raw_image_t* dest);

 private:
  /*!\brief compress gainmap image
   *
   * \param[in]       gainmap_img              gainmap image descriptor
   * \param[in]       jpeg_enc_obj             jpeg encoder object handle
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t compressGainMap(uhdr_raw_image_t* gainmap_img, JpegEncoderHelper* jpeg_enc_obj);

  /*!\brief This method is called to separate base image and gain map image from compressed
   * ultrahdr image
   *
   * \param[in]            jpegr_image               compressed ultrahdr image descriptor
   * \param[in, out]       primary_image             sdr image descriptor
   * \param[in, out]       gainmap_image             gainmap image descriptor
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t extractPrimaryImageAndGainMap(uhdr_compressed_image_t* jpegr_image,
                                                  uhdr_compressed_image_t* primary_image,
                                                  uhdr_compressed_image_t* gainmap_image);

  /*!\brief This function parses the bitstream and returns metadata that is useful for actual
   * decoding. This does not decode the image. That is handled by decompressImage().
   *
   * \param[in]            jpeg_image      compressed jpeg image descriptor
   * \param[in, out]       image_info      image info descriptor
   * \param[in, out]       img_width       (optional) image width
   * \param[in, out]       img_height      (optional) image height
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t parseJpegInfo(uhdr_compressed_image_t* jpeg_image, j_info_ptr image_info,
                                  size_t* img_width = nullptr, size_t* img_height = nullptr);

  /*!\brief This method takes compressed sdr intent, compressed gainmap coefficient, gainmap
   * metadata and creates a ultrahdr image. This is done by first generating XMP packet from gainmap
   * metadata, then appending in the order,
   *    SOI, APP2 (Exif is present), APP2 (XMP), base image, gain map image.
   *
   * NOTE: In the final output, EXIF package will appear if ONLY ONE of the following conditions is
   * fulfilled:
   * (1) EXIF package is available from outside input. I.e. pExif != nullptr.
   * (2) Compressed sdr intent has EXIF.
   * If both conditions are fulfilled, this method will return error indicating that it is unable to
   * choose which exif to be placed in the bitstream.
   *
   * \param[in]       sdr_intent_compressed    sdr intent image descriptor
   * \param[in]       gainmap_compressed       gainmap intent input image descriptor
   * \param[in]       pExif                    exif block to be placed in the bitstream
   * \param[in]       pIcc                     pointer to icc segment that needs to be added to the
   *                                           compressed image
   * \param[in]       icc_size                 size of icc segment
   * \param[in]       metadata                 gainmap metadata descriptor
   * \param[in, out]  dest                     output image descriptor to store compressed ultrahdr
   *                                           image
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t appendGainMap(uhdr_compressed_image_t* sdr_intent_compressed,
                                  uhdr_compressed_image_t* gainmap_compressed,
                                  uhdr_mem_block_t* pExif, void* pIcc, size_t icc_size,
                                  uhdr_gainmap_metadata_ext_t* metadata,
                                  uhdr_compressed_image_t* dest);

  /*!\brief This method is used to tone map a hdr image
   *
   * \param[in]            hdr_intent      hdr image descriptor
   * \param[in, out]       sdr_intent      sdr image descriptor
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t toneMap(uhdr_raw_image_t* hdr_intent, uhdr_raw_image_t* sdr_intent);

  /*!\brief This method is used to convert a raw image from one gamut space to another gamut space
   * in-place.
   *
   * \param[in, out]  image              raw image descriptor
   * \param[in]       src_encoding       input gamut space
   * \param[in]       dst_encoding       destination gamut space
   *
   * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
   */
  uhdr_error_info_t convertYuv(uhdr_raw_image_t* image, uhdr_color_gamut_t src_encoding,
                               uhdr_color_gamut_t dst_encoding);

  /*
   * This method will check the validity of the input arguments.
   *
   * @param p010_image_ptr uncompressed HDR image in P010 color format
   * @param yuv420_image_ptr pointer to uncompressed SDR image struct. HDR image is expected to
   *                         be in 420p color format
   * @param hdr_tf transfer function of the HDR image
   * @param dest destination of the compressed JPEGR image. Please note that {@code maxLength}
   *             represents the maximum available size of the desitination buffer, and it must be
   *             set before calling this method. If the encoded JPEGR size exceeds
   *             {@code maxLength}, this method will return {@code ERROR_JPEGR_BUFFER_TOO_SMALL}.
   * @return NO_ERROR if the input args are valid, error code is not valid.
   */
  status_t areInputArgumentsValid(jr_uncompressed_ptr p010_image_ptr,
                                  jr_uncompressed_ptr yuv420_image_ptr,
                                  ultrahdr_transfer_function hdr_tf, jr_compressed_ptr dest_ptr);

  /*
   * This method will check the validity of the input arguments.
   *
   * @param p010_image_ptr uncompressed HDR image in P010 color format
   * @param yuv420_image_ptr pointer to uncompressed SDR image struct. HDR image is expected to
   *                         be in 420p color format
   * @param hdr_tf transfer function of the HDR image
   * @param dest destination of the compressed JPEGR image. Please note that {@code maxLength}
   *             represents the maximum available size of the destination buffer, and it must be
   *             set before calling this method. If the encoded JPEGR size exceeds
   *             {@code maxLength}, this method will return {@code ERROR_JPEGR_BUFFER_TOO_SMALL}.
   * @param quality target quality of the JPEG encoding, must be in range of 0-100 where 100 is
   *                the highest quality
   * @return NO_ERROR if the input args are valid, error code is not valid.
   */
  status_t areInputArgumentsValid(jr_uncompressed_ptr p010_image_ptr,
                                  jr_uncompressed_ptr yuv420_image_ptr,
                                  ultrahdr_transfer_function hdr_tf, jr_compressed_ptr dest,
                                  int quality);

  // Configurations
  void* mUhdrGLESCtxt;              // opengl es context
  size_t mMapDimensionScaleFactor;  // gain map scale factor
  int mMapCompressQuality;          // gain map quality factor
  bool mUseMultiChannelGainMap;     // enable multichannel gain map
  float mGamma;                     // gain map gamma parameter
  uhdr_enc_preset_t mEncPreset;     // encoding speed preset
  float mMinContentBoost;           // min content boost recommendation
  float mMaxContentBoost;           // max content boost recommendation
};

struct GlobalTonemapOutputs {
  std::array<float, 3> rgb_out;
  float y_hdr;
  float y_sdr;
};

// Applies a global tone mapping, based on Chrome's HLG/PQ rendering implemented
// at
// https://source.chromium.org/chromium/chromium/src/+/main:ui/gfx/color_transform.cc;l=1198-1232;drc=ac505aff1d29ec3bfcf317cb77d5e196a3664e92
// `rgb_in` is expected to be in the normalized range of [0.0, 1.0] and
// `rgb_out` is returned in this same range. `headroom` describes the ratio
// between the HDR and SDR peak luminances and must be > 1. The `y_sdr` output
// is in the range [0.0, 1.0] while `y_hdr` is in the range [0.0, headroom].
GlobalTonemapOutputs globalTonemap(const std::array<float, 3>& rgb_in, float headroom,
                                   float luminance);

}  // namespace ultrahdr

#endif  // ULTRAHDR_JPEGR_H
