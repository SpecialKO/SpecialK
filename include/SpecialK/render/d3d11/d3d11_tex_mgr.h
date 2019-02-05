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
#include <com_util.h>

static const GUID SKID_D3D11Texture2D_DISCARD =
// {5C5298CA-0F9C-4931-A19D-A2E69792AE02}
{ 0x5c5298ca, 0xf9c,  0x4931, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x2 } };

struct cache_params_s {
  uint32_t max_entries       = 4096UL;
  uint32_t min_entries       = 1024UL;
  uint32_t max_size          = 2048UL; // Measured in MiB
  uint32_t min_size          = 512UL;
  uint32_t min_evict         = 16;
  uint32_t max_evict         = 64;
      bool ignore_non_mipped = false;
} extern cache_opts;

extern CRITICAL_SECTION tex_cs;
extern CRITICAL_SECTION hash_cs;
extern CRITICAL_SECTION dump_cs;
extern CRITICAL_SECTION cache_cs;
extern CRITICAL_SECTION inject_cs;
extern CRITICAL_SECTION preload_cs;

extern std::wstring SK_D3D11_res_root;
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
                                     ID3D11DeviceContext*  pDevCtx = (ID3D11DeviceContext *)SK_GetCurrentRenderBackend ().d3d11.immediate_ctx.p,
                                     ID3D11Device*         pDev    = (ID3D11Device        *)SK_GetCurrentRenderBackend ().device.p );

HRESULT __stdcall SK_D3D11_DumpTexture2D       (_In_ ID3D11Texture2D* pTex, uint32_t crc32c);
BOOL              SK_D3D11_DeleteDumpedTexture (uint32_t crc32c);