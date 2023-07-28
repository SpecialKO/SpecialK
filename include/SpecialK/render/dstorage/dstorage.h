/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#ifndef __SK__DSTORAGE_H__
#define __SK__DSTORAGE_H__

#include <Unknwnbase.h>

enum DSTORAGE_COMPRESSION_FORMAT {
  DSTORAGE_COMPRESSION_FORMAT_NONE     = 0,
  DSTORAGE_COMPRESSION_FORMAT_GDEFLATE = 1,
  DSTORAGE_CUSTOM_COMPRESSION_0        = 0x80
};

struct DSTORAGE_CONFIGURATION {
  UINT32 NumSubmitThreads;
  INT32  NumBuiltInCpuDecompressionThreads;
  BOOL   ForceMappingLayer;
  BOOL   DisableBypassIO;
  BOOL   DisableTelemetry;
  BOOL   DisableGpuDecompressionMetacommand;
  BOOL   DisableGpuDecompression;
};

struct DSTORAGE_CONFIGURATION1 : DSTORAGE_CONFIGURATION {
  BOOL   ForceFileBuffering;
};

using DStorageSetConfiguration_pfn  = HRESULT (WINAPI *)(DSTORAGE_CONFIGURATION  const *configuration );
using DStorageSetConfiguration1_pfn = HRESULT (WINAPI *)(DSTORAGE_CONFIGURATION1 const *configuration1);

using DStorageCreateCompressionCodec_pfn = HRESULT (WINAPI *)(DSTORAGE_COMPRESSION_FORMAT format,
                                                              UINT32                      numThreads,
                                                              REFIID                      riid,
                                                              void                      **ppv);

using DStorageGetFactory_pfn = HRESULT (WINAPI *)(REFIID,void **);

void SK_DStorage_Init            (void);
bool SK_DStorage_IsLoaded        (void);
bool SK_DStorage_IsUsingGDeflate (void);

#endif /* __SK__DSTORAGE_H__ */