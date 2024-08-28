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

/** \file ultrahdr_api.h
 *
 *  \brief
 *  Describes the encoder or decoder algorithm interface to applications.
 */

#ifndef ULTRAHDR_API_H
#define ULTRAHDR_API_H

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(UHDR_BUILDING_SHARED_LIBRARY)
#define UHDR_API __declspec(dllexport)
#elif defined(UHDR_USING_SHARED_LIBRARY)
#define UHDR_API __declspec(dllimport)
#else
#define UHDR_API
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 4) && \
    (defined(UHDR_BUILDING_SHARED_LIBRARY) || defined(UHDR_USING_SHARED_LIBRARY))
#define UHDR_API __attribute__((visibility("default")))
#else
#define UHDR_API
#endif

#ifdef __cplusplus
#define UHDR_EXTERN extern "C" UHDR_API
#else
#define UHDR_EXTERN extern UHDR_API
#endif

// ===============================================================================================
// Enum Definitions
// ===============================================================================================

/*!\brief List of supported image formats */
typedef enum uhdr_img_fmt {
  UHDR_IMG_FMT_UNSPECIFIED = -1,   /**< Unspecified */
  UHDR_IMG_FMT_24bppYCbCrP010 = 0, /**< 10-bit-per component 4:2:0 YCbCr semiplanar format.
                               Each chroma and luma component has 16 allocated bits in
                               little-endian configuration with 10 MSB of actual data.*/
  UHDR_IMG_FMT_12bppYCbCr420 = 1,  /**< 8-bit-per component 4:2:0 YCbCr planar format */
  UHDR_IMG_FMT_8bppYCbCr400 = 2,   /**< 8-bit-per component Monochrome format */
  UHDR_IMG_FMT_32bppRGBA8888 =
      3, /**< 32 bits per pixel RGBA color format, with 8-bit red, green, blue
        and alpha components. Using 32-bit little-endian representation,
        colors stored as Red 7:0, Green 15:8, Blue 23:16, Alpha 31:24. */
  UHDR_IMG_FMT_64bppRGBAHalfFloat = 4, /**< 64 bits per pixel RGBA color format, with 16-bit signed
                                   floating point red, green, blue, and alpha components */
  UHDR_IMG_FMT_32bppRGBA1010102 = 5,   /**< 32 bits per pixel RGBA color format, with 10-bit red,
                                      green,   blue, and 2-bit alpha components. Using 32-bit
                                      little-endian   representation, colors stored as Red 9:0, Green
                                      19:10, Blue   29:20, and Alpha 31:30. */
  UHDR_IMG_FMT_24bppYCbCr444 = 6,      /**< 8-bit-per component 4:4:4 YCbCr planar format */
  UHDR_IMG_FMT_16bppYCbCr422 = 7,      /**< 8-bit-per component 4:2:2 YCbCr planar format */
  UHDR_IMG_FMT_16bppYCbCr440 = 8,      /**< 8-bit-per component 4:4:0 YCbCr planar format */
  UHDR_IMG_FMT_12bppYCbCr411 = 9,      /**< 8-bit-per component 4:1:1 YCbCr planar format */
  UHDR_IMG_FMT_10bppYCbCr410 = 10,     /**< 8-bit-per component 4:1:0 YCbCr planar format */
  UHDR_IMG_FMT_24bppRGB888 = 11,       /**< 8-bit-per component RGB interleaved format */
  UHDR_IMG_FMT_30bppYCbCr444 = 12,     /**< 10-bit-per component 4:4:4 YCbCr planar format */
} uhdr_img_fmt_t;                      /**< alias for enum uhdr_img_fmt */

/*!\brief List of supported color gamuts */
typedef enum uhdr_color_gamut {
  UHDR_CG_UNSPECIFIED = -1, /**< Unspecified */
  UHDR_CG_BT_709 = 0,       /**< BT.709 */
  UHDR_CG_DISPLAY_P3 = 1,   /**< Display P3 */
  UHDR_CG_BT_2100 = 2,      /**< BT.2100 */
} uhdr_color_gamut_t;       /**< alias for enum uhdr_color_gamut */

/*!\brief List of supported color transfers */
typedef enum uhdr_color_transfer {
  UHDR_CT_UNSPECIFIED = -1, /**< Unspecified */
  UHDR_CT_LINEAR = 0,       /**< Linear */
  UHDR_CT_HLG = 1,          /**< Hybrid log gamma */
  UHDR_CT_PQ = 2,           /**< Perceptual Quantizer */
  UHDR_CT_SRGB = 3,         /**< Gamma */
} uhdr_color_transfer_t;    /**< alias for enum uhdr_color_transfer */

/*!\brief List of supported color ranges */
typedef enum uhdr_color_range {
  UHDR_CR_UNSPECIFIED = -1,  /**< Unspecified */
  UHDR_CR_LIMITED_RANGE = 0, /**< Y {[16..235], UV [16..240]} * pow(2, (bpc - 8)) */
  UHDR_CR_FULL_RANGE = 1,    /**< YUV/RGB {[0..255]} * pow(2, (bpc - 8)) */
} uhdr_color_range_t;        /**< alias for enum uhdr_color_range */

/*!\brief List of supported codecs */
typedef enum uhdr_codec {
  UHDR_CODEC_JPG,  /**< Compress {Hdr, Sdr rendition} to an {Sdr rendition + Gain Map} using jpeg */
  UHDR_CODEC_HEIF, /**< Compress {Hdr, Sdr rendition} to an {Sdr rendition + Gain Map} using heif */
  UHDR_CODEC_AVIF, /**< Compress {Hdr, Sdr rendition} to an {Sdr rendition + Gain Map} using avif */
} uhdr_codec_t;    /**< alias for enum uhdr_codec */

/*!\brief Image identifiers in gain map technology */
typedef enum uhdr_img_label {
  UHDR_HDR_IMG,      /**< Hdr rendition image */
  UHDR_SDR_IMG,      /**< Sdr rendition image */
  UHDR_BASE_IMG,     /**< Base rendition image */
  UHDR_GAIN_MAP_IMG, /**< Gain map image */
} uhdr_img_label_t;  /**< alias for enum uhdr_img_label */

/*!\brief uhdr encoder usage parameter */
typedef enum uhdr_enc_preset {
  UHDR_USAGE_REALTIME,     /**< tune encoder settings for performance */
  UHDR_USAGE_BEST_QUALITY, /**< tune encoder settings for quality */
} uhdr_enc_preset_t;       /**< alias for enum uhdr_enc_preset */

/*!\brief Algorithm return codes */
typedef enum uhdr_codec_err {

  /*!\brief Operation completed without error */
  UHDR_CODEC_OK,

  /*!\brief Generic codec error, refer detail field for more information */
  UHDR_CODEC_ERROR,

  /*!\brief Unknown error, refer detail field for more information */
  UHDR_CODEC_UNKNOWN_ERROR,

  /*!\brief An application-supplied parameter is not valid. */
  UHDR_CODEC_INVALID_PARAM,

  /*!\brief Memory operation failed */
  UHDR_CODEC_MEM_ERROR,

  /*!\brief An application-invoked operation is not valid */
  UHDR_CODEC_INVALID_OPERATION,

  /*!\brief The library does not implement a feature required for the operation */
  UHDR_CODEC_UNSUPPORTED_FEATURE,

  /*!\brief Not for usage, indicates end of list */
  UHDR_CODEC_LIST_END,

} uhdr_codec_err_t; /**< alias for enum uhdr_codec_err */

/*!\brief List of supported mirror directions. */
typedef enum uhdr_mirror_direction {
  UHDR_MIRROR_VERTICAL,    /**< flip image over x axis */
  UHDR_MIRROR_HORIZONTAL,  /**< flip image over y axis */
} uhdr_mirror_direction_t; /**< alias for enum uhdr_mirror_direction */

// ===============================================================================================
// Structure Definitions
// ===============================================================================================

/*!\brief Detailed return status */
typedef struct uhdr_error_info {
  uhdr_codec_err_t error_code;
  int has_detail;
  char detail[256];
} uhdr_error_info_t; /**< alias for struct uhdr_error_info */

/**\brief Raw Image Descriptor */
typedef struct uhdr_raw_image {
  /* Color Aspects: Color model, primaries, transfer, range */
  uhdr_img_fmt_t fmt;       /**< Image Format */
  uhdr_color_gamut_t cg;    /**< Color Gamut */
  uhdr_color_transfer_t ct; /**< Color Transfer */
  uhdr_color_range_t range; /**< Color Range */

  /* Image storage dimensions */
  unsigned int w; /**< Stored image width */
  unsigned int h; /**< Stored image height */

  /* Image data pointers. */
#define UHDR_PLANE_PACKED 0 /**< To be used for all packed formats */
#define UHDR_PLANE_Y 0      /**< Y (Luminance) plane */
#define UHDR_PLANE_U 1      /**< U (Chroma) plane */
#define UHDR_PLANE_UV 1     /**< UV (Chroma plane interleaved) To be used for semi planar format */
#define UHDR_PLANE_V 2      /**< V (Chroma) plane */
  void* planes[3];          /**< pointer to the top left pixel for each plane */
  unsigned int stride[3];   /**< stride in pixels between rows for each plane */
} uhdr_raw_image_t;         /**< alias for struct uhdr_raw_image */

/**\brief Compressed Image Descriptor */
typedef struct uhdr_compressed_image {
  void* data;               /**< Pointer to a block of data to decode */
  unsigned int data_sz;     /**< size of the data buffer */
  unsigned int capacity;    /**< maximum size of the data buffer */
  uhdr_color_gamut_t cg;    /**< Color Gamut */
  uhdr_color_transfer_t ct; /**< Color Transfer */
  uhdr_color_range_t range; /**< Color Range */
} uhdr_compressed_image_t;  /**< alias for struct uhdr_compressed_image */

/**\brief Buffer Descriptor */
typedef struct uhdr_mem_block {
  void* data;            /**< Pointer to a block of data to decode */
  unsigned int data_sz;  /**< size of the data buffer */
  unsigned int capacity; /**< maximum size of the data buffer */
} uhdr_mem_block_t;      /**< alias for struct uhdr_mem_block */

/**\brief Gain map metadata. */
typedef struct uhdr_gainmap_metadata {
  float max_content_boost; /**< Value to control how much brighter an image can get, when shown on
                              an HDR display, relative to the SDR rendition. This is constant for a
                              given image. Value MUST be in linear scale. */
  float min_content_boost; /**< Value to control how much darker an image can get, when shown on
                              an HDR display, relative to the SDR rendition. This is constant for a
                              given image. Value MUST be in linear scale. */
  float gamma;             /**< Encoding Gamma of the gainmap image. */
  float offset_sdr; /**< The offset to apply to the SDR pixel values during gainmap generation and
                       application. */
  float offset_hdr; /**< The offset to apply to the HDR pixel values during gainmap generation and
                       application. */
  float hdr_capacity_min;  /**< Minimum display boost value for which the map is applied completely.
                              Value MUST be in linear scale. */
  float hdr_capacity_max;  /**< Maximum display boost value for which the map is applied completely.
                              Value MUST be in linear scale. */
} uhdr_gainmap_metadata_t; /**< alias for struct uhdr_gainmap_metadata */

/**\brief ultrahdr codec context opaque descriptor */
typedef struct uhdr_codec_private uhdr_codec_private_t;

// ===============================================================================================
// Function Declarations
// ===============================================================================================

// ===============================================================================================
// Encoder APIs
// ===============================================================================================

/*!\brief Create a new encoder instance. The instance is initialized with default settings.
 * To override the settings use uhdr_enc_set_*()
 *
 * \return  nullptr if there was an error allocating memory else a fresh opaque encoder handle
 */
UHDR_EXTERN uhdr_codec_private_t* uhdr_create_encoder(void);

/*!\brief Release encoder instance.
 * Frees all allocated storage associated with encoder instance.
 *
 * \param[in]  enc  encoder instance.
 *
 * \return none
 */
UHDR_EXTERN void uhdr_release_encoder(uhdr_codec_private_t* enc);

/*!\brief Add raw image descriptor to encoder context. The function goes through all the fields of
 * the image descriptor and checks for their sanity. If no anomalies are seen then the image is
 * added to internal list. Repeated calls to this function will replace the old entry with the
 * current.
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  img  image descriptor.
 * \param[in]  intent  UHDR_HDR_IMG for hdr intent and UHDR_SDR_IMG for sdr intent.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enc_set_raw_image(uhdr_codec_private_t* enc,
                                                     uhdr_raw_image_t* img,
                                                     uhdr_img_label_t intent);

/*!\brief Add compressed image descriptor to encoder context. The function goes through all the
 * fields of the image descriptor and checks for their sanity. If no anomalies are seen then the
 * image is added to internal list. Repeated calls to this function will replace the old entry with
 * the current.
 *
 * If both uhdr_enc_add_raw_image() and uhdr_enc_add_compressed_image() are called during a session
 * for the same intent, it is assumed that raw image descriptor and compressed image descriptor are
 * relatable via compress <-> decompress process.
 *
 * The compressed image descriptors has fields cg, ct and range. Certain media formats are capable
 * of storing color standard, color transfer and color range characteristics in the bitstream (for
 * example heif, avif, ...). Other formats may not support this (jpeg, ...). These fields serve as
 * an additional source for conveying this information. If the user is unaware of the color aspects
 * of the image, #UHDR_CG_UNSPECIFIED, #UHDR_CT_UNSPECIFIED, #UHDR_CR_UNSPECIFIED can be used. If
 * color aspects are present inside the bitstream and supplied via these fields both are expected to
 * be identical.
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  img  image descriptor.
 * \param[in]  intent  UHDR_HDR_IMG for hdr intent,
 *                     UHDR_SDR_IMG for sdr intent,
 *                     UHDR_BASE_IMG for base image intent
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enc_set_compressed_image(uhdr_codec_private_t* enc,
                                                            uhdr_compressed_image_t* img,
                                                            uhdr_img_label_t intent);

/*!\brief Add gain map image descriptor and gainmap metadata info that was used to generate the
 * aforth gainmap image to encoder context. The function internally goes through all the fields of
 * the image descriptor and checks for their sanity. If no anomalies are seen then the image is
 * added to internal list. Repeated calls to this function will replace the old entry with the
 * current.
 *
 * NOTE: There are apis that allow configuration of gainmap info separately. For instance
 * #uhdr_enc_set_gainmap_gamma, #uhdr_enc_set_gainmap_scale_factor, ... They have no effect on the
 * information that is configured via this api. The information configured here is treated as
 * immutable and used as-is in encoding scenario where gainmap computations are intended to be
 * by-passed.
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  img  gain map image desciptor.
 * \param[in]  metadata  gainmap metadata descriptor
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enc_set_gainmap_image(uhdr_codec_private_t* enc,
                                                         uhdr_compressed_image_t* img,
                                                         uhdr_gainmap_metadata_t* metadata);

/*!\brief Set quality factor for compressing base image and/or gainmap image. Default configured
 * quality factor of base image and gainmap image are 95 and 85 respectively.
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  quality  quality factor. Any integer in range [0 - 100].
 * \param[in]  intent  #UHDR_BASE_IMG for base image and #UHDR_GAIN_MAP_IMG for gain map image.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enc_set_quality(uhdr_codec_private_t* enc, int quality,
                                                   uhdr_img_label_t intent);

/*!\brief Set Exif data that needs to be inserted in the output compressed stream. This function
 * does not generate or validate exif data on its own. It merely copies the supplied information
 * into the bitstream.
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  img  exif data memory block.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enc_set_exif_data(uhdr_codec_private_t* enc,
                                                     uhdr_mem_block_t* exif);

/*!\brief Enable/Disable multi-channel gainmap. By default single-channel gainmap is enabled.
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  use_multi_channel_gainmap. 0 - single-channel gainmap is enabled,
 *                                        otherwise - multi-channel gainmap is enabled.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t
uhdr_enc_set_using_multi_channel_gainmap(uhdr_codec_private_t* enc, int use_multi_channel_gainmap);

/*!\brief Set gain map scaling factor. The encoding process allows signalling a downscaled gainmap
 * image instead of full resolution. This setting controls the factor by which the renditions are
 * downscaled. For instance, gain_map_scale_factor = 2 implies gainmap_image_width =
 * primary_image_width / 2 and gainmap image height = primary_image_height / 2.
 * Default gain map scaling factor is 4.
 * NOTE: This has no effect on base image rendition. Base image is signalled in full resolution
 * always.
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  gain_map_scale_factor  gain map scale factor. Any integer in range (0, 128]
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enc_set_gainmap_scale_factor(uhdr_codec_private_t* enc,
                                                                int gain_map_scale_factor);

/*!\brief Set encoding gamma of gainmap image. For multi-channel gainmap image, set gamma is used
 * for gamma correction of all planes separately. Default gamma value is 1.0.
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  gamma  gamma of gainmap image. Any positive real number > 0.0.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enc_set_gainmap_gamma(uhdr_codec_private_t* enc, float gamma);

/*!\brief Set min max content boost. This configuration is treated as a recommendation by the
 * library. It is entirely possible for the library to use a different set of values. Value MUST be
 * in linear scale.
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  min_boost min content boost. Any positive real number > 0.0f
 * \param[in]  max_boost max content boost. Any positive real number >= min_boost
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enc_set_min_max_content_boost(uhdr_codec_private_t* enc,
                                                                 float min_boost, float max_boost);

/*!\brief Set encoding preset. Tunes the encoder configurations for performance or quality. Default
 * configuration is #UHDR_USAGE_REALTIME.
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  preset  encoding preset. #UHDR_USAGE_REALTIME - Tune settings for best performance
 *                                      #UHDR_USAGE_BEST_QUALITY - Tune settings for best quality
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enc_set_preset(uhdr_codec_private_t* enc,
                                                  uhdr_enc_preset_t preset);

/*!\brief Set output image compression format. Selects the compression format for encoding base
 * image and gainmap image. Default configuration is #UHDR_CODEC_JPG
 *
 * \param[in]  enc  encoder instance.
 * \param[in]  media_type  output image compression format. Supported values are #UHDR_CODEC_JPG
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enc_set_output_format(uhdr_codec_private_t* enc,
                                                         uhdr_codec_t media_type);

/*!\brief Encode process call
 * After initializing the encoder context, call to this function will submit data for encoding. If
 * the call is successful, the encoded output is stored internally and is accessible via
 * uhdr_get_encoded_stream().
 *
 * The basic usage of uhdr encoder is as follows:
 * - The program creates an instance of an encoder using,
 *   - uhdr_create_encoder().
 * - The program registers input images to the encoder using,
 *   - uhdr_enc_set_raw_image(ctxt, img, UHDR_HDR_IMG)
 *   - uhdr_enc_set_raw_image(ctxt, img, UHDR_SDR_IMG)
 * - The program overrides the default settings using uhdr_enc_set_*() functions
 * - If the application wants to control the compression level
 *   - uhdr_enc_set_quality()
 * - If the application wants to insert exif data
 *   - uhdr_enc_set_exif_data()
 * - If the application wants to set gainmap scale factor
 *   - uhdr_enc_set_gainmap_scale_factor()
 * - If the application wants to enable multi channel gain map
 *   - uhdr_enc_set_using_multi_channel_gainmap()
 * - If the application wants to set gainmap image gamma
 *   - uhdr_enc_set_gainmap_gamma()
 * - If the application wants to set encoding preset
 *   - uhdr_enc_set_preset()
 * - If the application wants to control target compression format
 *   - uhdr_enc_set_output_format()
 * - The program calls uhdr_encode() to encode data. This call would initiate the process of
 * computing gain map from hdr intent and sdr intent. The sdr intent and gain map image are
 * compressed at the set quality using the codec of choice.
 * - On success, the program can access the encoded output with uhdr_get_encoded_stream().
 * - The program finishes the encoding with uhdr_release_encoder().
 *
 * The library allows setting Hdr and/or Sdr intent in compressed format,
 * - uhdr_enc_set_compressed_image(ctxt, img, UHDR_HDR_IMG)
 * - uhdr_enc_set_compressed_image(ctxt, img, UHDR_SDR_IMG)
 * In this mode, the compressed image(s) are first decoded to raw image(s). These raw image(s) go
 * through the aforth mentioned gain map computation and encoding process. In this case, the usage
 * shall be like this:
 * - uhdr_create_encoder()
 * - uhdr_enc_set_compressed_image(ctxt, img, UHDR_HDR_IMG)
 * - uhdr_enc_set_compressed_image(ctxt, img, UHDR_SDR_IMG)
 * - uhdr_encode()
 * - uhdr_get_encoded_stream()
 * - uhdr_release_encoder()
 * If the set compressed image media type of intent UHDR_SDR_IMG and output media type are
 * identical, then this image is directly used for primary image. No re-encode of raw image is done.
 * This implies base image quality setting is un-used. Only gain map image is encoded at the set
 * quality using codec of choice. On the other hand, if the set compressed image media type and
 * output media type are different, then transcoding is done.
 *
 * The library also allows directly setting base and gain map image in compressed format,
 * - uhdr_enc_set_compressed_image(ctxt, img, UHDR_BASE_IMG)
 * - uhdr_enc_set_gainmap_image(ctxt, img, metadata)
 * In this mode, gain map computation is by-passed. The input images are transcoded (if necessary),
 * combined and sent back.
 *
 * It is possible to create a uhdr image solely from Hdr intent. In this case, the usage shall look
 * like this:
 * - uhdr_create_encoder()
 * - uhdr_enc_set_raw_image(ctxt, img, UHDR_HDR_IMG)
 * - uhdr_enc_set_quality() // optional
 * - uhdr_enc_set_exif_data() // optional
 * - uhdr_enc_set_output_format() // optional
 * - uhdr_enc_set_gainmap_scale_factor() // optional
 * - uhdr_enc_set_using_multi_channel_gainmap() // optional
 * - uhdr_enc_set_gainmap_gamma() // optional
 * - uhdr_encode()
 * - uhdr_get_encoded_stream()
 * - uhdr_release_encoder()
 * In this mode, the Sdr rendition is created from Hdr intent by tone-mapping. The tone-mapped sdr
 * image and hdr image go through the aforth mentioned gain map computation and encoding process to
 * create uhdr image.
 *
 * In all modes, Exif data is inserted if requested.
 *
 * \param[in]  enc  encoder instance.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_encode(uhdr_codec_private_t* enc);

/*!\brief Get encoded ultra hdr stream
 *
 * \param[in]  enc  encoder instance.
 *
 * \return nullptr if encode process call is unsuccessful, uhdr image descriptor otherwise
 */
UHDR_EXTERN uhdr_compressed_image_t* uhdr_get_encoded_stream(uhdr_codec_private_t* enc);

/*!\brief Reset encoder instance.
 * Clears all previous settings and resets to default state and ready for re-initialization and
 * usage
 *
 * \param[in]  enc  encoder instance.
 *
 * \return none
 */
UHDR_EXTERN void uhdr_reset_encoder(uhdr_codec_private_t* enc);

// ===============================================================================================
// Decoder APIs
// ===============================================================================================

/*!\brief check if it is a valid ultrahdr image.
 *
 * @param[in]  data  pointer to input compressed stream
 * @param[in]  size  size of compressed stream
 *
 * @returns 1 if the input data has a primary image, gain map image and gain map metadata. 0 if any
 *          errors are encountered during parsing process or if the image does not have primary
 *          image or gainmap image or gainmap metadata
 */
UHDR_EXTERN int is_uhdr_image(void* data, int size);

/*!\brief Create a new decoder instance. The instance is initialized with default settings.
 * To override the settings use uhdr_dec_set_*()
 *
 * \return  nullptr if there was an error allocating memory else a fresh opaque decoder handle
 */
UHDR_EXTERN uhdr_codec_private_t* uhdr_create_decoder(void);

/*!\brief Release decoder instance.
 * Frees all allocated storage associated with decoder instance.
 *
 * \param[in]  dec  decoder instance.
 *
 * \return none
 */
UHDR_EXTERN void uhdr_release_decoder(uhdr_codec_private_t* dec);

/*!\brief Add compressed image descriptor to decoder context. The function goes through all the
 * fields of the image descriptor and checks for their sanity. If no anomalies are seen then the
 * image is added to internal list. Repeated calls to this function will replace the old entry with
 * the current.
 *
 * \param[in]  dec  decoder instance.
 * \param[in]  img  image descriptor.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_dec_set_image(uhdr_codec_private_t* dec,
                                                 uhdr_compressed_image_t* img);

/*!\brief Set output image color format
 *
 * \param[in]  dec  decoder instance.
 * \param[in]  fmt  output image color format. Supported values are
 *                  #UHDR_IMG_FMT_64bppRGBAHalfFloat, #UHDR_IMG_FMT_32bppRGBA1010102,
 *                  #UHDR_IMG_FMT_32bppRGBA8888
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_dec_set_out_img_format(uhdr_codec_private_t* dec,
                                                          uhdr_img_fmt_t fmt);

/*!\brief Set output image color transfer characteristics. It should be noted that not all
 * combinations of output color format and output transfer function are supported. #UHDR_CT_SRGB
 * output color transfer shall be paired with #UHDR_IMG_FMT_32bppRGBA8888 only. #UHDR_CT_HLG,
 * #UHDR_CT_PQ shall be paired with #UHDR_IMG_FMT_32bppRGBA1010102. #UHDR_CT_LINEAR shall be paired
 * with #UHDR_IMG_FMT_64bppRGBAHalfFloat.
 *
 * \param[in]  dec  decoder instance.
 * \param[in]  ct  output color transfer
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_dec_set_out_color_transfer(uhdr_codec_private_t* dec,
                                                              uhdr_color_transfer_t ct);

/*!\brief Set output display's HDR capacity. Value MUST be in linear scale. This value determines
 * the weight by which the gain map coefficients are scaled. If no value is configured, no weight is
 * applied to gainmap image.
 *
 * \param[in]  dec  decoder instance.
 * \param[in]  display_boost  hdr capacity of target display. Any real number >= 1.0f
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds,
 *                           #UHDR_CODEC_INVALID_PARAM otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_dec_set_out_max_display_boost(uhdr_codec_private_t* dec,
                                                                 float display_boost);

/*!\brief This function parses the bitstream that is registered with the decoder context and makes
 * image information available to the client via uhdr_dec_get_() functions. It does not decompress
 * the image. That is done by uhdr_decode().
 *
 * \param[in]  dec  decoder instance.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_dec_probe(uhdr_codec_private_t* dec);

/*!\brief Get base image width
 *
 * \param[in]  dec  decoder instance.
 *
 * \return -1 if probe call is unsuccessful, base image width otherwise
 */
UHDR_EXTERN int uhdr_dec_get_image_width(uhdr_codec_private_t* dec);

/*!\brief Get base image height
 *
 * \param[in]  dec  decoder instance.
 *
 * \return -1 if probe call is unsuccessful, base image height otherwise
 */
UHDR_EXTERN int uhdr_dec_get_image_height(uhdr_codec_private_t* dec);

/*!\brief Get gainmap image width
 *
 * \param[in]  dec  decoder instance.
 *
 * \return -1 if probe call is unsuccessful, gain map image width otherwise
 */
UHDR_EXTERN int uhdr_dec_get_gainmap_width(uhdr_codec_private_t* dec);

/*!\brief Get gainmap image height
 *
 * \param[in]  dec  decoder instance.
 *
 * \return -1 if probe call is unsuccessful, gain map image height otherwise
 */
UHDR_EXTERN int uhdr_dec_get_gainmap_height(uhdr_codec_private_t* dec);

/*!\brief Get exif information
 *
 * \param[in]  dec  decoder instance.
 *
 * \return nullptr if probe call is unsuccessful, memory block with exif data otherwise
 */
UHDR_EXTERN uhdr_mem_block_t* uhdr_dec_get_exif(uhdr_codec_private_t* dec);

/*!\brief Get icc information
 *
 * \param[in]  dec  decoder instance.
 *
 * \return nullptr if probe call is unsuccessful, memory block with icc data otherwise
 */
UHDR_EXTERN uhdr_mem_block_t* uhdr_dec_get_icc(uhdr_codec_private_t* dec);

/*!\brief Get gain map metadata
 *
 * \param[in]  dec  decoder instance.
 *
 * \return nullptr if probe process call is unsuccessful, gainmap metadata descriptor otherwise
 */
UHDR_EXTERN uhdr_gainmap_metadata_t* uhdr_dec_get_gain_map_metadata(uhdr_codec_private_t* dec);

/*!\brief Decode process call
 * After initializing the decoder context, call to this function will submit data for decoding. If
 * the call is successful, the decoded output is stored internally and is accessible via
 * uhdr_get_decoded_image().
 *
 * The basic usage of uhdr decoder is as follows:
 * - The program creates an instance of a decoder using,
 *   - uhdr_create_decoder().
 * - The program registers input images to the decoder using,
 *   - uhdr_dec_set_image(ctxt, img)
 * - The program overrides the default settings using uhdr_dec_set_*() functions.
 * - If the application wants to control the output image format,
 *   - uhdr_dec_set_out_img_format()
 * - If the application wants to control the output transfer characteristics,
 *   - uhdr_dec_set_out_color_transfer()
 * - If the application wants to control the output display boost,
 *   - uhdr_dec_set_out_max_display_boost()
 * - If the application wants to enable/disable gpu acceleration,
 *   - uhdr_enable_gpu_acceleration()
 * - The program calls uhdr_decode() to decode uhdr stream. This call would initiate the process
 * of decoding base image and gain map image. These two are combined to give the final rendition
 * image.
 * - The program can access the decoded output with uhdr_get_decoded_image().
 * - The program finishes the decoding with uhdr_release_decoder().
 *
 * \param[in]  dec  decoder instance.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, uhdr_codec_err_t otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_decode(uhdr_codec_private_t* dec);

/*!\brief Get final rendition image
 *
 * \param[in]  dec  decoder instance.
 *
 * \return nullptr if decoded process call is unsuccessful, raw image descriptor otherwise
 */
UHDR_EXTERN uhdr_raw_image_t* uhdr_get_decoded_image(uhdr_codec_private_t* dec);

/*!\brief Get gain map image
 *
 * \param[in]  dec  decoder instance.
 *
 * \return nullptr if decoded process call is unsuccessful, raw image descriptor otherwise
 */
UHDR_EXTERN uhdr_raw_image_t* uhdr_get_gain_map_image(uhdr_codec_private_t* dec);

/*!\brief Reset decoder instance.
 * Clears all previous settings and resets to default state and ready for re-initialization and
 * usage
 *
 * \param[in]  dec  decoder instance.
 *
 * \return none
 */
UHDR_EXTERN void uhdr_reset_decoder(uhdr_codec_private_t* dec);

// ===============================================================================================
// Common APIs
// ===============================================================================================

/*!\brief Enable/Disable GPU acceleration.
 * If enabled, certain operations (if possible) of uhdr encode/decode will be offloaded to GPU.
 * NOTE: It is entirely possible for this API to have no effect on the encode/decode operation
 *
 * \param[in]  codec  codec instance.
 * \param[in]  enable  choice
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, #UHDR_CODEC_INVALID_PARAM
 * otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_enable_gpu_acceleration(uhdr_codec_private_t* codec, int enable);

/*!\brief Add image editing operations (pre-encode or post-decode).
 * Below functions list the set of edits supported. Program can set any combination of these during
 * initialization. Once the encode/decode process call is made, before encoding or after decoding
 * the edits are applied in the order of configuration.
 */

/*!\brief Add mirror effect
 *
 * \param[in]  codec  codec instance.
 * \param[in]  direction  mirror directions. #UHDR_MIRROR_VERTICAL for vertical mirroring
 *                                           #UHDR_MIRROR_HORIZONTAL for horizontal mirroing
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, #UHDR_CODEC_INVALID_PARAM
 * otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_add_effect_mirror(uhdr_codec_private_t* codec,
                                                     uhdr_mirror_direction_t direction);

/*!\brief Add rotate effect
 *
 * \param[in]  codec  codec instance.
 * \param[in]  degrees  clockwise degrees. 90 - rotate clockwise by 90 degrees
 *                                         180 - rotate clockwise by 180 degrees
 *                                         270 - rotate clockwise by 270 degrees
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, #UHDR_CODEC_INVALID_PARAM
 * otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_add_effect_rotate(uhdr_codec_private_t* codec, int degrees);

/*!\brief Add crop effect
 *
 * \param[in]  codec  codec instance.
 * \param[in]  left  crop coordinate left in pixels.
 * \param[in]  right  crop coordinate right in pixels.
 * \param[in]  top  crop coordinate top in pixels.
 * \param[in]  bottom  crop coordinate bottom in pixels.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, #UHDR_CODEC_INVALID_PARAM
 * otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_add_effect_crop(uhdr_codec_private_t* codec, int left, int right,
                                                   int top, int bottom);

/*!\brief Add resize effect
 *
 * \param[in]  codec  codec instance.
 * \param[in]  width  target width.
 * \param[in]  height  target height.
 *
 * \return uhdr_error_info_t #UHDR_CODEC_OK if operation succeeds, #UHDR_CODEC_INVALID_PARAM
 * otherwise.
 */
UHDR_EXTERN uhdr_error_info_t uhdr_add_effect_resize(uhdr_codec_private_t* codec, int width,
                                                     int height);

#endif  // ULTRAHDR_API_H
