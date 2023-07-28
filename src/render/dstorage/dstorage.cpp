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

#include <SpecialK/stdafx.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L" DStorage "

#include <SpecialK/render/dstorage/dstorage.h>

DStorageCreateCompressionCodec_pfn DStorageCreateCompressionCodec_Original = nullptr;
DStorageGetFactory_pfn             DStorageGetFactory_Original             = nullptr;
DStorageSetConfiguration_pfn       DStorageSetConfiguration_Original       = nullptr;
DStorageSetConfiguration1_pfn      DStorageSetConfiguration1_Original      = nullptr;

bool SK_DStorage_UsingDLL      = false;
bool SK_DStorage_UsingGDeflate = false;

void
SK_DStorage_ApplyConfigOverrides (DSTORAGE_CONFIGURATION *pConfig)
{
  if (config.render.dstorage.disable_bypass_io)
    pConfig->DisableBypassIO = true;

  if (config.render.dstorage.disable_gpu_decomp)
    pConfig->DisableGpuDecompression = true;

  if (config.render.dstorage.disable_telemetry)
    pConfig->DisableTelemetry = true;
}

void
SK_DStorage_ApplyConfigOverrides1 (DSTORAGE_CONFIGURATION1 *pConfig1)
{
  SK_DStorage_ApplyConfigOverrides (pConfig1);

  if (config.render.dstorage.force_file_buffering)
    pConfig1->ForceFileBuffering = true;
}

HRESULT
WINAPI
DStorageSetConfiguration_Detour (DSTORAGE_CONFIGURATION const *configuration)
{
  SK_LOG_FIRST_CALL

  if (! configuration)
    return E_POINTER;

  auto                               cfg = *configuration;
  SK_DStorage_ApplyConfigOverrides (&cfg);

  return
    DStorageSetConfiguration_Original (&cfg);
}

HRESULT
WINAPI
DStorageSetConfiguration1_Detour (DSTORAGE_CONFIGURATION1 const *configuration1)
{
  SK_LOG_FIRST_CALL

  if (! configuration1)
    return E_POINTER;

  auto                                cfg1 = *configuration1;
  SK_DStorage_ApplyConfigOverrides1 (&cfg1);

  return
    DStorageSetConfiguration1_Original (&cfg1);
}

HRESULT
WINAPI
DStorageCreateCompressionCodec_Detour (
  DSTORAGE_COMPRESSION_FORMAT format,
  UINT32                      numThreads,
  REFIID                      riid,
  void                      **ppv )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    DStorageCreateCompressionCodec_Original (format, numThreads, riid, ppv);

  if (SUCCEEDED (hr))
  {
    if (format == DSTORAGE_COMPRESSION_FORMAT_GDEFLATE)
      SK_DStorage_UsingGDeflate = true;

    SK_RunOnce (
      SK_LOGi0 (L"Game is using GDeflate!")
    );
  }

  return hr;
}

HRESULT
WINAPI
DStorageGetFactory_Detour ( REFIID riid,
                            void **ppv )
{
  SK_LOG_FIRST_CALL

  return
    DStorageGetFactory_Original (riid, ppv);
}

void SK_DStorage_Init (void)
{
  SK_DStorage_UsingDLL =
    GetModuleHandleW (L"dstorage.dll") != nullptr;

  if (SK_DStorage_IsLoaded ())
  {
    SK_RunOnce (
    {
      SK_CreateDLLHook2 (L"dstorage.dll", "DStorageCreateCompressionCodec",
                                           DStorageCreateCompressionCodec_Detour,
                  static_cast_p2p <void> (&DStorageCreateCompressionCodec_Original) );

      SK_CreateDLLHook2 (L"dstorage.dll", "DStorageGetFactory",
                                           DStorageGetFactory_Detour,
                  static_cast_p2p <void> (&DStorageGetFactory_Original) );

      SK_CreateDLLHook2 (L"dstorage.dll", "DStorageSetConfiguration",
                                           DStorageSetConfiguration_Detour,
                  static_cast_p2p <void> (&DStorageSetConfiguration_Original) );

      SK_CreateDLLHook2 (L"dstorage.dll", "DStorageSetConfiguration1",
                                           DStorageSetConfiguration1_Detour,
                  static_cast_p2p <void> (&DStorageSetConfiguration1_Original) );

      SK_ApplyQueuedHooks ();
    });
  }
}

bool SK_DStorage_IsLoaded (void)
{
  return
    SK_DStorage_UsingDLL;
}

bool SK_DStorage_IsUsingGDeflate (void)
{
  return
    SK_DStorage_UsingGDeflate;
}