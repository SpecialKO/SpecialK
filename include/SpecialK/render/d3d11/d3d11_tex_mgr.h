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
#pragma once

#include <com_util.h>

extern const GUID SKID_D3D11Texture2D_DISCARD;

struct cache_params_s {
  uint32_t max_entries       = 4096UL;
  uint32_t min_entries       = 1024UL;
  uint32_t max_size          = 2048UL; // Measured in MiB
  uint32_t min_size          = 512UL;
  uint32_t min_evict         = 16;
  uint32_t max_evict         = 64;
      bool ignore_non_mipped = false;
} extern cache_opts;

// Temporary staging for memory-mapped texture uploads
//
struct mapped_resources_s {
  std::unordered_map <ID3D11Resource*,  D3D11_MAPPED_SUBRESOURCE> textures;
  std::unordered_map <ID3D11Resource*,  uint64_t>                 texture_times;

  std::unordered_map <ID3D11Resource*,  uint32_t>                 dynamic_textures;
  std::unordered_map <ID3D11Resource*,  uint32_t>                 dynamic_texturesx;
  std::map           <uint32_t,         uint64_t>                 dynamic_times2;
  std::map           <uint32_t,         size_t>                   dynamic_sizes2;
};

struct SK_D3D11_TEXTURE2D_DESC
{
  UINT                  Width;
  UINT                  Height;
  UINT                  MipLevels;
  UINT                  ArraySize;
  DXGI_FORMAT           Format;
  DXGI_SAMPLE_DESC      SampleDesc;
  D3D11_USAGE           Usage;
  D3D11_BIND_FLAG       BindFlags;
  D3D11_CPU_ACCESS_FLAG CPUAccessFlags;
  UINT                  MiscFlags;

  explicit SK_D3D11_TEXTURE2D_DESC (D3D11_TEXTURE2D_DESC& descFrom)
  {
    Width          = descFrom.Width;
    Height         = descFrom.Height;
    MipLevels      = descFrom.MipLevels;
    ArraySize      = descFrom.ArraySize;
    Format         = descFrom.Format;
    SampleDesc     = descFrom.SampleDesc;
    Usage          = descFrom.Usage;
    BindFlags      = (D3D11_BIND_FLAG      )descFrom.BindFlags;
    CPUAccessFlags = (D3D11_CPU_ACCESS_FLAG)descFrom.CPUAccessFlags;
    MiscFlags      = descFrom.MiscFlags;
  }
};

extern CRITICAL_SECTION tex_cs;
extern CRITICAL_SECTION hash_cs;
extern CRITICAL_SECTION dump_cs;
extern CRITICAL_SECTION cache_cs;
extern CRITICAL_SECTION inject_cs;
extern CRITICAL_SECTION preload_cs;

extern SK_LazyGlobal <std::wstring>
                    SK_D3D11_res_root;
extern bool         SK_D3D11_need_tex_reset;
extern bool         SK_D3D11_try_tex_reset;
extern int32_t      SK_D3D11_amount_to_purge;

extern bool         SK_D3D11_dump_textures;
extern bool         SK_D3D11_inject_textures_ffx;
extern bool         SK_D3D11_inject_textures;
extern bool         SK_D3D11_cache_textures;
extern bool         SK_D3D11_mark_textures;

extern
  volatile LONG     SK_D3D11_TexRefCount_Failures;

extern std::wstring SK_D3D11_TexNameFromChecksum (uint32_t top_crc32, uint32_t checksum, uint32_t ffx_crc32 = 0x00);
extern bool         SK_D3D11_IsTexInjectThread   (SK_TLS *pTLS = SK_TLS_Bottom ());


void __stdcall SK_D3D11_AddInjectable    (uint32_t top_crc32,  uint32_t checksum);
void __stdcall SK_D3D11_RemoveInjectable (uint32_t top_crc32,  uint32_t checksum);
void __stdcall SK_D3D11_AddTexHash       (const wchar_t* name, uint32_t top_crc32, uint32_t hash);

bool __stdcall SK_D3D11_IsDumped         (uint32_t top_crc32, uint32_t checksum);
bool __stdcall SK_D3D11_IsInjectable     (uint32_t top_crc32, uint32_t checksum);
bool __stdcall SK_D3D11_IsInjectable_FFX (uint32_t top_crc32);

int     SK_D3D11_ReloadAllTextures (void);
HRESULT SK_D3D11_ReloadTexture     ( ID3D11Texture2D* pTex,
                                     SK_TLS*          pTLS = SK_TLS_Bottom () );

HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2DEx ( const DirectX::ScratchImage&   img,
                                        uint32_t                 crc32c,
                                        ID3D11Texture2D*       /*pOutTex*/,
                                        DirectX::ScratchImage** ppOutImg,
                                        SK_TLS*                  pTLS );

HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2D ( _In_ ID3D11Texture2D*      pTex,
                                     uint32_t              crc32c,
                                     SK_TLS*               pTLS = SK_TLS_Bottom (),
                                     ID3D11DeviceContext*  pDevCtx = (ID3D11DeviceContext *)SK_GetCurrentRenderBackend ().d3d11.immediate_ctx,
                                     ID3D11Device*         pDev    = (ID3D11Device        *)SK_GetCurrentRenderBackend ().device.p );

HRESULT __stdcall SK_D3D11_DumpTexture2D       (_In_ ID3D11Texture2D* pTex, uint32_t crc32c);
BOOL              SK_D3D11_DeleteDumpedTexture (uint32_t crc32c);

bool
SK_D3D11_IsStagingCacheable ( D3D11_RESOURCE_DIMENSION  rdim,
                              ID3D11Resource           *pRes,
                              SK_TLS                   *pTLS = nullptr );