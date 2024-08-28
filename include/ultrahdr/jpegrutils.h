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

#ifndef ULTRAHDR_JPEGRUTILS_H
#define ULTRAHDR_JPEGRUTILS_H

#include "ultrahdr/jpegr.h"

// TODO (dichenzhang): This is old version metadata, new version can be found in
// https://drive.google.com/file/d/1yUGmjGytRuBa2vpr9eM5Uu8CVhyyddjp/view?resourcekey=0-HGzFrzPQzu5FNYLRAJXQBA
// and in gainmapmetadata.h/.cpp
// This file is kept in order to keep the backward compatibility.
namespace ultrahdr {

static constexpr uint32_t EndianSwap32(uint32_t value) {
  return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) |
         (value >> 24);
}
static inline uint16_t EndianSwap16(uint16_t value) {
  return static_cast<uint16_t>((value >> 8) | ((value & 0xFF) << 8));
}

/*
 * Mutable data structure. Holds information for metadata.
 */
class DataStruct {
 private:
  void* data;
  int writePos;
  int length;

 public:
  DataStruct(int s);
  ~DataStruct();

  void* getData();
  int getLength();
  int getBytesWritten();
  bool write8(uint8_t value);
  bool write16(uint16_t value);
  bool write32(uint32_t value);
  bool write(const void* src, int size);
};

/*
 * Helper function used for writing data to destination.
 *
 * @param destination destination of the data to be written.
 * @param source source of data being written.
 * @param length length of the data to be written.
 * @param position cursor in desitination where the data is to be written.
 * @return success or error code.
 */
uhdr_error_info_t Write(uhdr_compressed_image_t* destination, const void* source, int length,
                        int& position);

/*
 * Parses XMP packet and fills metadata with data from XMP
 *
 * @param xmp_data pointer to XMP packet
 * @param xmp_size size of XMP packet
 * @param metadata place to store HDR metadata values
 * @return success or error code.
 */
uhdr_error_info_t getMetadataFromXMP(uint8_t* xmp_data, int xmp_size,
                                     uhdr_gainmap_metadata_ext_t* metadata);

/*
 * This method generates XMP metadata for the primary image.
 *
 * below is an example of the XMP metadata that this function generates where
 * secondary_image_length = 1000
 *
 * <x:xmpmeta
 *   xmlns:x="adobe:ns:meta/"
 *   x:xmptk="Adobe XMP Core 5.1.2">
 *   <rdf:RDF
 *     xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
 *     <rdf:Description
 *       xmlns:Container="http://ns.google.com/photos/1.0/container/"
 *       xmlns:Item="http://ns.google.com/photos/1.0/container/item/"
 *       xmlns:hdrgm="http://ns.adobe.com/hdr-gain-map/1.0/"
 *       hdrgm:Version="1">
 *       <Container:Directory>
 *         <rdf:Seq>
 *           <rdf:li
 *             rdf:parseType="Resource">
 *             <Container:Item
 *               Item:Semantic="Primary"
 *               Item:Mime="image/jpeg"/>
 *           </rdf:li>
 *           <rdf:li
 *             rdf:parseType="Resource">
 *             <Container:Item
 *               Item:Semantic="GainMap"
 *               Item:Mime="image/jpeg"
 *               Item:Length="1000"/>
 *           </rdf:li>
 *         </rdf:Seq>
 *       </Container:Directory>
 *     </rdf:Description>
 *   </rdf:RDF>
 * </x:xmpmeta>
 *
 * @param secondary_image_length length of secondary image
 * @return XMP metadata in type of string
 */
std::string generateXmpForPrimaryImage(int secondary_image_length,
                                       uhdr_gainmap_metadata_ext_t& metadata);

/*
 * This method generates XMP metadata for the recovery map image.
 * Link: https://developer.android.com/media/platform/hdr-image-format#XMP-attributes
 *
 * below is an example of the XMP metadata that this function generates where
 * max_content_boost = 8.0
 * min_content_boost = 0.5
 *
 * <x:xmpmeta
 *   xmlns:x="adobe:ns:meta/"
 *   x:xmptk="Adobe XMP Core 5.1.2">
 *   <rdf:RDF
 *     xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
 *     <rdf:Description
 *       xmlns:hdrgm="http://ns.adobe.com/hdr-gain-map/1.0/"
 *       hdrgm:Version="1"
 *       hdrgm:GainMapMin="-1"
 *       hdrgm:GainMapMax="3"
 *       hdrgm:Gamma="1"
 *       hdrgm:OffsetSDR="0"
 *       hdrgm:OffsetHDR="0"
 *       hdrgm:HDRCapacityMin="0"
 *       hdrgm:HDRCapacityMax="3"
 *       hdrgm:BaseRenditionIsHDR="False"/>
 *   </rdf:RDF>
 * </x:xmpmeta>
 *
 * @param metadata JPEG/R metadata to encode as XMP
 * @return XMP metadata in type of string
 */
std::string generateXmpForSecondaryImage(uhdr_gainmap_metadata_ext_t& metadata);

}  // namespace ultrahdr

#endif  // ULTRAHDR_JPEGRUTILS_H
