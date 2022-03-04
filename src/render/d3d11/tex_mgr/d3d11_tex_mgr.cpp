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
#define __SK_SUBSYSTEM__ L"D3D11TxMgr"

#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/dxgi/dxgi_util.h>

#include <execution>

#define _L2(w)  L ## w
#define  _L(w) _L2(w)

std::unique_ptr <SK_Thread_HybridSpinlock> tex_cs     = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> hash_cs    = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> dump_cs    = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> cache_cs   = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> inject_cs  = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> preload_cs = nullptr;

cache_params_s cache_opts   = { };

bool         SK_D3D11_need_tex_reset      = false;
bool         SK_D3D11_try_tex_reset       = false; // Don't need, but would be beneficial to try
int32_t      SK_D3D11_amount_to_purge     = 0L;

bool         SK_D3D11_dump_textures       = false;// true;
bool         SK_D3D11_inject_textures_ffx = false;
bool         SK_D3D11_inject_textures     = false;
bool         SK_D3D11_cache_textures      = false;
bool         SK_D3D11_mark_textures       = false;

SK_LazyGlobal <std::wstring> SK_D3D11_res_root;

const GUID SKID_D3D11Texture2D_DISCARD =
// {5C5298CA-0F9C-4931-A19D-A2E69792AE02}
{ 0x5c5298ca, 0xf9c,  0x4931,
  { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x2 } };

const GUID SKID_D3D11Texture2D_BaseObj =
// {5C5298CA-0F9C-4931-A19D-A2E69792AC02}
{ 0x5c5298ca, 0xf9c,  0x4931,
  { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xac, 0x2 } };

void WINAPI SK_D3D11_SetResourceRoot      (const wchar_t* root)  ;
void WINAPI SK_D3D11_EnableTexDump        (bool enable)          ;
void WINAPI SK_D3D11_EnableTexInject      (bool enable)          ;
void WINAPI SK_D3D11_EnableTexCache       (bool enable)          ;
void WINAPI SK_D3D11_PopulateResourceList (bool refresh = false) ;

void
SK_D3D11_InitTextures (void)
{
  static auto *pCommandProcessor =
    SK_GetCommandProcessor ();

  static volatile LONG SK_D3D11_tex_init = FALSE;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (FALSE == InterlockedCompareExchangeAcquire (&SK_D3D11_tex_init, TRUE, FALSE))
  {
    if (pTLS != nullptr)
        pTLS->render->d3d11->ctx_init_thread = TRUE;

    static bool bFFX =
      (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyX_X2);

    if (bFFX)
      SK_D3D11_inject_textures_ffx = true;

    preload_cs = std::make_unique <SK_Thread_HybridSpinlock> (0x01);
    dump_cs    = std::make_unique <SK_Thread_HybridSpinlock> (0x02);
    inject_cs  = std::make_unique <SK_Thread_HybridSpinlock> (0x10);
    hash_cs    = std::make_unique <SK_Thread_HybridSpinlock> (0x40);
    cache_cs   = std::make_unique <SK_Thread_HybridSpinlock> (0x80);
    tex_cs     = std::make_unique <SK_Thread_HybridSpinlock> (0xFF);

    cache_opts.max_entries       = config.textures.cache.max_entries;
    cache_opts.min_entries       = config.textures.cache.min_entries;
    cache_opts.max_evict         = config.textures.cache.max_evict;
    cache_opts.min_evict         = config.textures.cache.min_evict;
    cache_opts.max_size          = config.textures.cache.max_size;
    cache_opts.min_size          = config.textures.cache.min_size;
    cache_opts.ignore_non_mipped = config.textures.cache.ignore_nonmipped;

    //
    // Legacy Hack for Untitled Project X (FFX/FFX-2)
    //
  //extern bool SK_D3D11_inject_textures_ffx;
    if       (! SK_D3D11_inject_textures_ffx)
    {
      SK_D3D11_EnableTexCache  (config.textures.d3d11.cache);
      SK_D3D11_EnableTexDump   (config.textures.d3d11.dump);
      SK_D3D11_EnableTexInject (config.textures.d3d11.inject);
      SK_D3D11_SetResourceRoot (config.textures.d3d11.res_root.c_str ());
    }

    pCommandProcessor->AddVariable ("TexCache.Enable",
         new SK_IVarStub <bool> ((bool *)&config.textures.d3d11.cache));
    pCommandProcessor->AddVariable ("TexCache.MaxEntries",
         new SK_IVarStub <int>  ((int *)&cache_opts.max_entries));
    pCommandProcessor->AddVariable ("TexC ache.MinEntries",
         new SK_IVarStub <int>  ((int *)&cache_opts.min_entries));
    pCommandProcessor->AddVariable ("TexCache.MaxSize",
         new SK_IVarStub <int>  ((int *)&cache_opts.max_size));
    pCommandProcessor->AddVariable ("TexCache.MinSize",
         new SK_IVarStub <int>  ((int *)&cache_opts.min_size));
    pCommandProcessor->AddVariable ("TexCache.MinEvict",
         new SK_IVarStub <int>  ((int *)&cache_opts.min_evict));
    pCommandProcessor->AddVariable ("TexCache.MaxEvict",
         new SK_IVarStub <int>  ((int *)&cache_opts.max_evict));
    pCommandProcessor->AddVariable ("TexCache.IgnoreNonMipped",
         new SK_IVarStub <bool> ((bool *)&cache_opts.ignore_non_mipped));

    if ((! SK_D3D11_inject_textures_ffx) && config.textures.d3d11.inject)
      SK_D3D11_PopulateResourceList ();


    // XXX: Why is this here?
#ifdef _M_AMD64
    static bool bOkami =
      (SK_GetCurrentGameID () == SK_GAME_ID::Okami);

    if (bOkami)
    {
      extern void SK_Okami_LoadConfig (void);
                  SK_Okami_LoadConfig ();
    }
#endif

    InterlockedIncrementRelease (&SK_D3D11_tex_init);
  }

  else if (pTLS != nullptr && (! pTLS->render->d3d11->ctx_init_thread))
    SK_Thread_SpinUntilAtomicMin (&SK_D3D11_tex_init, 2);
}

bool SK_D3D11_IsTexInjectThread (SK_TLS *pTLS)
{
  if (pTLS == nullptr)
      pTLS  = SK_TLS_Bottom ();

  return
    ( pTLS->texture_management.injection_thread != FALSE );
}

void
SK_D3D11_ClearTexInjectThread (SK_TLS *pTLS = SK_TLS_Bottom ())
{
  if (pTLS == nullptr)
      pTLS  = SK_TLS_Bottom ();

  pTLS->texture_management.injection_thread = FALSE;
}

void
SK_D3D11_SetTexInjectThread (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  if (pTLS == nullptr)
      pTLS  = SK_TLS_Bottom ();

  pTLS->texture_management.injection_thread = TRUE;
}

SK_ScopedBoolFwd
SK_D3D11_DeclareTexInjectScope (SK_TLS *pTLS)
{
  if (pTLS == nullptr)
      pTLS  = SK_TLS_Bottom ();

  BOOL bOrig =
    pTLS->texture_management.injection_thread;

  SK_D3D11_SetTexInjectThread (pTLS);

  return
    std::make_pair ( &pTLS->texture_management.injection_thread,
                      bOrig );
}


using IUnknown_Release_pfn =
ULONG (WINAPI *)(IUnknown* This);
using IUnknown_AddRef_pfn  =
ULONG (WINAPI *)(IUnknown* This);

IUnknown_Release_pfn IUnknown_Release_Original = nullptr;
IUnknown_AddRef_pfn  IUnknown_AddRef_Original  = nullptr;

volatile LONG SK_D3D11_TestingRefCounts     = FALSE;
volatile LONG SK_D3D11_TexRefCount_Failures = 0;
//
// If reference counting is broken by some weird COM wrapper
//   misbehaving, then for the love of ... don't cache textures
//     using SK *==> it hooks rather than wraps.
//
BOOL
SK_D3D11_TestRefCountHooks ( ID3D11Texture2D *pInputTex,
                             SK_TLS          *pTLS = nullptr )
{
  if (pInputTex == nullptr)
    return FALSE;

  // This is unsafe, but ... the variable name already told you that!
  if (config.textures.cache.allow_unsafe_refs)
    return TRUE;

  if (pTLS == nullptr)
      pTLS = SK_TLS_Bottom ();

  SK_ScopedBool decl_tex_scope (
    SK_D3D11_DeclareTexInjectScope (pTLS)
  );

  auto                  orig_refcount_obj =
    pTLS->texture_management.refcount_obj;
    pTLS->texture_management.refcount_obj = pInputTex;

  auto SanityFail =
    [&](void)  ->
    BOOL
  {
    pTLS->texture_management.refcount_obj =
                        orig_refcount_obj;

    InterlockedDecrement (&SK_D3D11_TestingRefCounts);
    InterlockedIncrement (&SK_D3D11_TexRefCount_Failures);

    return FALSE;
  };

  InterlockedIncrement (&SK_D3D11_TestingRefCounts);

  LONG initial =
    pTLS->texture_management.refcount_test;

  pInputTex->AddRef ();

  const LONG initial_plus_one =
    pTLS->texture_management.refcount_test;

  pInputTex->Release ();

  const LONG initial_again =
    pTLS->texture_management.refcount_test;

  pTLS->texture_management.refcount_obj =
                      orig_refcount_obj;

  if ( initial != initial_plus_one - 1 ||
       initial != initial_again ) //-V560
  {
    SK_LOG1 ( (L"Expected %li after AddRef (); got %li.",
             initial + 1, initial_plus_one ),
             L"DX11TexMgr" );
    SK_LOG1 ( (L"Expected %li after Release (); got %li.",
             initial, initial_again ),
             L"DX11TexMgr" );

    return
      SanityFail ();
  }


  // Important note: The D3D11 runtime may implement QueryInterface on an
  //                   ID3D11Texture2D by returning a SEPARATE object with
  //                     an initial reference count of 1.
  //
  //     Effectively, QueryInterface creates a view with its own lifetime,
  //       never expect to get the original object by making this call!
  //

  pTLS->texture_management.refcount_test = 0;

  // Also validate that the wrapper's QueryInterface method is
  //   invoking our hooks
  //
  ID3D11Texture2D*                              pReferenced = nullptr;
  pInputTex->QueryInterface <ID3D11Texture2D> (&pReferenced);

  if (pReferenced == nullptr)
  {
    return
      SanityFail ();
  }

  pTLS->texture_management.refcount_obj = pReferenced;

  initial =
    pTLS->texture_management.refcount_test;

  pReferenced->Release ();

  const LONG initial_after_release =
    pTLS->texture_management.refcount_test;

  if ( initial != initial_after_release + 1 )
  {
    SK_LOG1 ( (L"Expected %li after QueryInterface (...); got %li.",
             initial + 1, initial_plus_one ),
             L"DX11TexMgr" );
    SK_LOG1 ( (L"Expected %li after Release (); got %li.",
             initial, initial_again ),
             L"DX11TexMgr" );

    return
      SanityFail ();
  }

  pTLS->texture_management.refcount_obj =
                      orig_refcount_obj;

  InterlockedDecrement (&SK_D3D11_TestingRefCounts);

  return TRUE;
}

__declspec (noinline)
ULONG
WINAPI
IUnknown_Release (IUnknown* This)
{
  SK_TLS* pTLS = nullptr;

  if (ReadAcquire (&SK_D3D11_TestingRefCounts) > 0)
  {
    pTLS =
      SK_TLS_Bottom ();

    // Objects destroyed during DLL detach have the potential to
    //   have no access to TLS, while still being obligated to release.
    if (pTLS == nullptr)
    {
      if (     IUnknown_Release_Original != nullptr)
        return IUnknown_Release_Original (This);

      return 0;
    }

    if ( SK_D3D11_IsTexInjectThread (pTLS) &&
         This ==                     pTLS->texture_management.refcount_obj )
    {
      pTLS->texture_management.refcount_test--;
    }
  }


#ifdef _M_AMD64
  static bool __yk =
    ( SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0 ||
      SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami2 ||
      SK_GetCurrentGameID () == SK_GAME_ID::YakuzaUnderflow );

  extern bool  __SK_Y0_SafetyLeak;
  if ( __yk && __SK_Y0_SafetyLeak )
  {
    LONG count =
      IUnknown_AddRef_Original  (This);
      IUnknown_Release_Original (This);

    // Prevent negative reference counts from causing an infinite loop
    if (count <= 2)
    {
      // If it's a shader, who the hell cares, these should remain loaded
      //   at all times anyway.
      SK_ComQIPtr <ID3D11VertexShader>   pVertexShader   (This);
      SK_ComQIPtr <ID3D11PixelShader>    pPixelShader    (This);
      SK_ComQIPtr <ID3D11GeometryShader> pGeometryShader (This);
      SK_ComQIPtr <ID3D11HullShader>     pHullShader     (This);
      SK_ComQIPtr <ID3D11DomainShader>   pDomainShader   (This);
      SK_ComQIPtr <ID3D11ComputeShader>  pComputeShader  (This);

      if ( pVertexShader || pPixelShader  || pGeometryShader ||
           pHullShader   || pDomainShader || pComputeShader )
      {
        pVertexShader.Detach   (); pPixelShader.Detach   ();
        pGeometryShader.Detach (); pHullShader.Detach    ();
        pDomainShader.Detach   (); pComputeShader.Detach ();

        return 0;
      }


      if (SK_GetCallingDLL () == skModuleRegistry::HostApp ())
      {
        // Negative values will happen because of this engine, and we
        //   can't fix the leak any more safely than the engine created it !
        if (count <= 1)
        {
          // But we can prevent the engine from counting down from
          //   4 billion to 0 while paint dries.
          return 0;
        }

        SK_ComQIPtr <ID3D11Resource>
            pRes (This);
        if (pRes.p != nullptr)
        {
          D3D11_RESOURCE_DIMENSION res_dim = { };
                 pRes.p->GetType (&res_dim);

          if (res_dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
          {
            SK_ComQIPtr <ID3D11Texture2D>
                pTex (pRes.p);
            if (pTex)
            {
              D3D11_TEXTURE2D_DESC  tex_desc = { };
                    pTex->GetDesc (&tex_desc);

              if ( (tex_desc.Usage    != D3D11_USAGE_IMMUTABLE) ||
                   (tex_desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) ==
                                         D3D11_BIND_UNORDERED_ACCESS)
              {
                // Detach and release so that SK_ComQIPtr <...> dtors unwind
                //   in the reverse order they were constructed.
                IUnknown_Release_Original (pTex.Detach ());
                IUnknown_Release_Original (pRes.Detach ());

                return
                  IUnknown_Release_Original (This);
              }

              else
              {
                return 0;
              }
            }
          }
        }

        // SK and ReShade both hold references to
        //   depth/stencil, render target and shader resource
        //     views but no other type of view.
        else
        {
          SK_ComQIPtr <ID3D11DepthStencilView>
              pDSV (This);
          if (pDSV.p != nullptr)
          {
            return 0;
          }

          SK_ComQIPtr <ID3D11RenderTargetView>
              pRTV (This);
          if (pRTV.p != nullptr)
          {
            return 0;
          }

          SK_ComQIPtr <ID3D11ShaderResourceView>
              pSRV (This);
          if (pSRV.p != nullptr)
          {
            return 0;
          }
        }
      }
    }


    //// If count is == 0, something's screwy
    //if (pTex != nullptr && count <= 2)
    //{
    //  if (SK_D3D11_RemoveTexFromCache (pTex))
    //  {
    //    if (count < 2)
    //      SK_LOG0 ( ( L"Unexpected reference count: %lu", count ), L"DX11TexMgr" );
    //
    //    IUnknown_Release_Original (pTex);
    //  }
    //}
  }
#endif

  return
    IUnknown_Release_Original (This);
}

__declspec (noinline)
ULONG
WINAPI
IUnknown_AddRef (IUnknown *This)
{
 SK_TLS* pTLS = nullptr;

  if (ReadAcquire (&SK_D3D11_TestingRefCounts) > 0)
  {
    pTLS =
      SK_TLS_Bottom ();

    // We are looking for exact interface pointer matches only;
    //   if we tested COM object equality the correct way
    //     from within here we would recurse infinitely!
    if ( SK_D3D11_IsTexInjectThread (pTLS) &&
         This ==                     pTLS->texture_management.refcount_obj )
    {
      pTLS->texture_management.refcount_test++;
    }
  }

  return
    IUnknown_AddRef_Original (This);
}


void
__stdcall
SK_D3D11_AddInjectable (uint32_t top_crc32, uint32_t checksum)
{
  static auto& textures =
    SK_D3D11_Textures;


  // Tag the hash, this is a collision avoidance / mitigation strategy
  //   but means every lookup / delete involves a simple 4-byte CRC32
  uint32_t tag = ( checksum == 0x0 ?
           0x0 :
    safe_crc32c (top_crc32, (uint8_t *)& checksum, 4) );


  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*inject_cs);

  if (tag != 0x00)
  { textures->injected_collisions.insert (tag);
  } textures->injectable_textures.insert (top_crc32);
}


void
__stdcall
SK_D3D11_RemoveInjectable (uint32_t top_crc32, uint32_t checksum)
{
  static auto& textures =
    SK_D3D11_Textures;

  // Tag the hash, this is a collision avoidance / mitigation strategy
  //   but means every lookup / delete involves a simple 4-byte CRC32
  uint32_t tag = ( checksum == 0x0 ?
           0x0 :
    safe_crc32c (top_crc32, (uint8_t *)& checksum, 4) );


  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*inject_cs);

  if (tag != 0x00)
  {
    textures->injected_collisions.erase (tag);
  }
  textures->injectable_textures.erase   (top_crc32);
}


bool
WINAPI
SK_D3D11_IsTexHashed (uint32_t top_crc32, uint32_t hash)
{
  static auto& textures =
    SK_D3D11_Textures.get ();

  static auto& tex_hashes    = textures.tex_hashes;
  static auto& tex_hashes_ex = textures.tex_hashes_ex;

  if ( tex_hashes_ex.empty () &&
       tex_hashes.empty    () )
  {
    return false;
  }

  // Tag the hash, this is a collision avoidance / mitigation strategy
  //   but means every lookup / delete involves a simple 4-byte CRC32
  const uint32_t tag =
    safe_crc32c (top_crc32, (const uint8_t *)&hash, 4);


  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*hash_cs);

  return (
    ( tex_hashes.find (tag)       != tex_hashes.cend () ) ||
    ( tex_hashes.find (top_crc32) != tex_hashes.cend () )
  );
}

void
WINAPI
SK_D3D11_AddTexHash ( const wchar_t* name, uint32_t top_crc32, uint32_t hash )
{
  // For early loading UnX
  SK_D3D11_InitTextures ();

  static auto& textures =
    SK_D3D11_Textures.get ();

  if (hash != 0x00)
  {
    if (! SK_D3D11_IsTexHashed (top_crc32, hash))
    {
      const uint32_t tag =
        safe_crc32c (top_crc32, (const uint8_t *)&hash, 4);

      {
        std::scoped_lock <SK_Thread_HybridSpinlock> critical (*hash_cs);
        textures.tex_hashes_ex.emplace (tag, name);
      }

      SK_D3D11_AddInjectable (top_crc32, hash);
    }
  }


  if (! SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
    {
      std::scoped_lock <SK_Thread_HybridSpinlock> critical (*hash_cs);
      textures.tex_hashes.emplace (top_crc32, name);
    }

    if (! SK_D3D11_inject_textures_ffx)
    {
      SK_D3D11_AddInjectable (top_crc32, 0x00);
    }

    else
    {
      std::scoped_lock <SK_Thread_HybridSpinlock> critical (*inject_cs);
      textures.injectable_ffx.emplace (top_crc32);
    }
  }
}

void
WINAPI
SK_D3D11_RemoveTexHash (uint32_t top_crc32, uint32_t hash)
{
  static auto& textures =
    SK_D3D11_Textures.get ();

  if ( textures.tex_hashes_ex.empty () &&
       textures.tex_hashes.empty    () )
  {
    return;
  }

  const
  uint32_t tag = ( hash == 0x0 ?
           0x0 :
    safe_crc32c (top_crc32, (const uint8_t *)&hash, 4) );


  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*hash_cs);

  if (     tag  != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash))
  {
    textures.tex_hashes_ex.erase (tag);

    SK_D3D11_RemoveInjectable (top_crc32, hash);
  }

  else if (hash == 0x00 && SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
    textures.tex_hashes.erase (top_crc32);

    SK_D3D11_RemoveInjectable (top_crc32, 0x00);
  }
}

std::wstring
__stdcall
SK_D3D11_TexHashToName (uint32_t top_crc32, uint32_t hash)
{
  static auto& textures =
    SK_D3D11_Textures.get ();

  if ( textures.tex_hashes_ex.empty () &&
       textures.tex_hashes.empty    () )
  {
    return L"";
  }

  uint32_t tag = ( hash == 0x0 ?
           0x0 :
    safe_crc32c (top_crc32, (const uint8_t *)&hash, 4) );

  std::wstring ret = L"";


  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*hash_cs);

  if (     tag  != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash))
  {
    ret =
      textures.tex_hashes_ex [tag];
  }

  else if (hash == 0x00 && SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
    ret =
      textures.tex_hashes [top_crc32];
  }

  return ret;
}


bool
__stdcall
SK_D3D11_IsDumped (uint32_t top_crc32, uint32_t checksum)
{
  static auto& dumped_textures   = SK_D3D11_Textures.get ().dumped_textures;
  static auto& dumped_collisions = SK_D3D11_Textures.get ().dumped_collisions;

  if (dumped_textures.empty ())
  {
    return false;
  }

  const uint32_t tag =
    safe_crc32c (top_crc32, (uint8_t *)&checksum, 4);


  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*dump_cs);

  if ( config.textures.d3d11.precise_hash &&
         dumped_collisions.cend (   ) !=
         dumped_collisions.find (tag)
     )
  {
    return true;
  }

  if (! config.textures.d3d11.precise_hash)
  {
    return
      ( dumped_textures.cend (         ) !=
        dumped_textures.find (top_crc32) );
  }

  return false;
}

void
__stdcall
SK_D3D11_AddDumped (uint32_t top_crc32, uint32_t checksum)
{
  static auto& dumped_textures   = SK_D3D11_Textures.get ().dumped_textures;
  static auto& dumped_collisions = SK_D3D11_Textures.get ().dumped_collisions;

  const uint32_t tag =
    safe_crc32c (top_crc32, (uint8_t *)&checksum, 4);


  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*dump_cs);

  if (! config.textures.d3d11.precise_hash)
  {
    dumped_textures.insert (top_crc32);
  }
  dumped_collisions.insert (tag);
}

void
__stdcall
SK_D3D11_RemoveDumped (uint32_t top_crc32, uint32_t checksum)
{
  static auto& dumped_textures   = SK_D3D11_Textures.get ().dumped_textures;
  static auto& dumped_collisions = SK_D3D11_Textures.get ().dumped_collisions;

  if (dumped_textures.empty ())
  {
    return;
  }

  const uint32_t tag =
    safe_crc32c (top_crc32, (uint8_t *)&checksum, 4);


  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*dump_cs);

  if (! config.textures.d3d11.precise_hash)
  {
    dumped_textures.erase (top_crc32);
  }
  dumped_collisions.erase (tag);
}

bool
__stdcall
SK_D3D11_IsInjectable (uint32_t top_crc32, uint32_t checksum)
{
  static auto& injectable_textures = SK_D3D11_Textures.get ().injectable_textures;
  static auto& injected_collisions = SK_D3D11_Textures.get ().injected_collisions;

  if (injectable_textures.empty ())
  {
    return false;
  }

  const uint32_t tag =
    safe_crc32c (top_crc32, (uint8_t *)&checksum, 4);


  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*inject_cs);

  if (checksum != 0x00)
  {
    return
      ( injected_collisions.cend (   ) !=
        injected_collisions.find (tag) );
  }

  return
    ( injectable_textures.cend (         ) !=
      injectable_textures.find (top_crc32) );
}

bool
__stdcall
SK_D3D11_IsInjectable_FFX (uint32_t top_crc32)
{
  static auto& injectable_ffx =
    SK_D3D11_Textures.get ().injectable_ffx;

  if (injectable_ffx.empty ())
  {
    return false;
  }

  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*inject_cs);

  return
    ( injectable_ffx.cend (         ) !=
      injectable_ffx.find (top_crc32) );
}

HRESULT
__stdcall
SK_D3D11_DumpTexture2D ( _In_ ID3D11Texture2D* pTex, uint32_t crc32c )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  auto pDev =
    rb.getDevice <ID3D11Device> ();

  SK_ComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

  if ( pDev    != nullptr &&
       pDevCtx != nullptr )
  {
    SK_ScopedBool decl_tex_scope (
      SK_D3D11_DeclareTexInjectScope ()
    );

    DirectX::ScratchImage img;

    if (SUCCEEDED (DirectX::CaptureTexture (pDev,
                                            pDevCtx, pTex, img)))
    {
      auto _IsTypeless = [&](void) ->
      bool { return DirectX::IsTypeless (img.GetMetadata ().format); };

      if (_IsTypeless ())
      {
        img.OverrideFormat   (
          DirectX::MakeTypelessUNORM   (
            img.GetMetadata ( ).format )
                             );

        if (_IsTypeless ())
        {
          img.OverrideFormat   (
            DirectX::MakeTypelessFLOAT   (
              img.GetMetadata ( ).format )
                               );
        }
      }

      wchar_t     wszPath [ MAX_PATH + 2 ] = { };
      wcsncpy_s ( wszPath,  MAX_PATH,
                  SK_EvalEnvironmentVars (SK_D3D11_res_root->c_str ()).c_str (),
                           _TRUNCATE );

      lstrcatW (wszPath, L"/dump/textures/");
      lstrcatW (wszPath, SK_GetHostApp ());
      lstrcatW (wszPath, L"/");

      const DXGI_FORMAT fmt =
        img.GetMetadata ().format;

      const bool compressed =
        SK_DXGI_IsFormatCompressed (fmt);

      wchar_t wszOutName [MAX_PATH + 2] = { };

      // Use filename to denote compression (not needed if metadata is spitout)
#if 1
      if (compressed)
      {
        swprintf ( wszOutName, LR"(%s\Compressed_%08X.dds)",
                     wszPath, crc32c );
      }

      else
      {
        swprintf ( wszOutName, LR"(%s\Uncompressed_%08X.dds)",
                     wszPath, crc32c );
      }
#else
      swprintf ( wszOutName, LR"(%s\%08X.dds)",
                   wszPath, crc32c );
#endif

      SK_CreateDirectories (wszPath);

      const HRESULT hr =
        DirectX::SaveToDDSFile ( img.GetImages   (), img.GetImageCount (),
                                 img.GetMetadata (), 0x00, wszOutName );

      if (SUCCEEDED (hr))
      {
        SK_D3D11_Textures->dumped_texture_bytes += SK_File_GetSize (wszOutName);

        SK_D3D11_AddDumped (crc32c, crc32c);


        wchar_t   wszMetaFilename [ MAX_PATH + 2 ] = { };
        swprintf (wszMetaFilename, L"%s.txt", wszOutName);

        FILE* fMetaData =
          _wfopen (wszMetaFilename, L"w+");

        if (fMetaData != nullptr)
        {
          D3D11_TEXTURE2D_DESC desc = { };
          pTex->GetDesc      (&desc);
          auto    pDesc   =   &desc;

          fprintf ( fMetaData,
                  "Dumped Name:    %ws.dds\n"
                  "Texture:        %08x\n"
                  "Dimensions:     (%ux%u)\n"
                  "Format:         %03u (%hs)\n"
                  "MipLODs:        %02u\n"
                  "Compressed:     %hs\n"
                  "sRGB:           %hs\n"
                  "----------------\n"
                  "BindFlags:      0x%04x\n"
                  "Usage:          0x%04x\n"
                  "CPUAccessFlags: 0x%02x\n"
                  "Misc:           0x%02x\n"
                  "ArraySize:      %02u",
                  wszOutName,
                    crc32c,
                      pDesc->Width, pDesc->Height,
                      pDesc->Format, SK_DXGI_FormatToStr (pDesc->Format).data (),
                        pDesc->MipLevels, compressed ?
                                               "Yes" : "No",
                        pDesc->Format ==
     DirectX::MakeSRGB (pDesc->Format) ?       "Yes" : "No",
                          pDesc->BindFlags,
                            pDesc->Usage,
                              pDesc->CPUAccessFlags,
                                pDesc->MiscFlags,
                                  pDesc->ArraySize );

          fclose (fMetaData);
        }

        return hr;
      }
    }
  }

  return E_FAIL;
}


HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2DEx ( DirectX::ScratchImage&   img,
                                  uint32_t                 crc32c,
                                  ID3D11Texture2D*       /*pOutTex*/,
                                  DirectX::ScratchImage** ppOutImg,
                                  SK_TLS*                  pTLS )
{
  UNREFERENCED_PARAMETER (pTLS);

  auto& metadata =
    img.GetMetadata ();

  if ( metadata.width  < 4 ||
       metadata.height < 4    ) return E_INVALIDARG;

  wchar_t     wszPath [ MAX_PATH + 2 ] = { };
  wcsncpy_s ( wszPath,  MAX_PATH,
              SK_EvalEnvironmentVars (
                SK_D3D11_res_root->c_str     ()
                                     ).c_str (), _TRUNCATE );

  lstrcatW             (wszPath, L"/inject/textures/MipmapCache/");
  lstrcatW             (wszPath, SK_GetHostApp ());
  lstrcatW             (wszPath, L"/");
  SK_CreateDirectories (wszPath);

  wchar_t    wszOutName [MAX_PATH + 2] = { };
  swprintf ( wszOutName, LR"(%s\%08X.dds)",
                       wszPath, crc32c );

  const bool compressed =
    SK_DXGI_IsFormatCompressed (
      metadata.format
    );

  DirectX::ScratchImage* mipmaps =
    nullptr;

  try
  {
    mipmaps =
      new DirectX::ScratchImage;
  }

  catch (const std::bad_alloc&)
  {
    mipmaps = nullptr;

    return
      E_OUTOFMEMORY;
  }

  HRESULT ret  = E_FAIL;
  size_t  size = 0;

  if (mipmaps && compressed)
  {
    DirectX::TexMetadata meta =
      img.GetMetadata ();

    img.OverrideFormat (
      SK_DXGI_MakeTypedFormat (meta.format)
    );

          DirectX::ScratchImage decompressed;
    const DirectX::Image*       orig_img =
      img.GetImage (0, 0, 0);

                             meta.format    =
    SK_DXGI_MakeTypedFormat (meta.format);
                             meta.mipLevels = 1;

    ret =
      DirectX::Decompress ( orig_img, 1, meta,
                              DXGI_FORMAT_UNKNOWN, decompressed );

    if (SUCCEEDED (ret))
    {
      ret =
       ( DirectX::GenerateMipMaps (
          decompressed.GetImage (0,0,0),
          1,
          decompressed.GetMetadata   (), DirectX::TEX_FILTER_BOX |
                                         DirectX::TEX_FILTER_FORCE_NON_WIC |
                                         DirectX::TEX_FILTER_SEPARATE_ALPHA,
          0, *mipmaps )
       );

      if (SUCCEEDED (ret))
      {
        if (! config.textures.d3d11.uncompressed_mips)
        {
          auto* compressed_mips =
            new DirectX::ScratchImage;

          DirectX::TexMetadata mipmap_meta =
            mipmaps->GetMetadata ();

          //
          // Required speedup since BC7 is _slow_
          //
          if (mipmap_meta.format == DXGI_FORMAT_BC7_UNORM)
              mipmap_meta.format = DXGI_FORMAT_BC5_UNORM;

                                   mipmap_meta.format =
          SK_DXGI_MakeTypedFormat (mipmap_meta.format);

          const DXGI_FORMAT newFormat =
            SK_DXGI_MakeTypedFormat (img.GetMetadata ().format);

          ret =
           ( DirectX::Compress ( mipmaps->GetImages       (),
                                   mipmaps->GetImageCount (),
                                     mipmap_meta,
                                       newFormat,
                                         DirectX::TEX_COMPRESS_DITHER //|
                                           ,//DirectX::TEX_COMPRESS_PARALLEL
                                           DirectX::TEX_THRESHOLD_DEFAULT,
                                             *compressed_mips )
           );

          if (SUCCEEDED (ret))
          {
            delete mipmaps;
                   mipmaps = compressed_mips;
          }

          else
            delete compressed_mips;
        }
      }
    }
  }

  else
  {
    if (mipmaps == nullptr)
        mipmaps  = new DirectX::ScratchImage;


    DirectX::TexMetadata meta =
      img.GetMetadata ();

    img.OverrideFormat (
      SK_DXGI_MakeTypedFormat (meta.format)
    );

    meta.format =
      SK_DXGI_MakeTypedFormat (meta.format);

    ret =
     ( DirectX::GenerateMipMaps (
         img.GetImages     (),
         img.GetImageCount (),
         meta,      DirectX::TEX_FILTER_BOX           |
                    DirectX::TEX_FILTER_FORCE_NON_WIC |
                    DirectX::TEX_FILTER_SEPARATE_ALPHA,
         0, *mipmaps )
     );
  }

  if ( SUCCEEDED (ret) &&
       mipmaps != nullptr )
  {
    if (config.textures.d3d11.cache_gen_mips)
    {
      DirectX::TexMetadata meta =
        mipmaps->GetMetadata ();

      meta.format =
        SK_DXGI_MakeTypedFormat (meta.format);

      if ( SUCCEEDED ( DirectX::SaveToDDSFile (
                         mipmaps->GetImages     (),
                         mipmaps->GetImageCount (),
                           meta, 0x00, wszOutName )
                     )
         )
      {
        size =
          mipmaps->GetPixelsSize ();
      }
    }
  }


  if (SUCCEEDED (ret))
  {
    if (ppOutImg != nullptr)
      *ppOutImg = mipmaps;

    delete mipmaps;

    if (config.textures.d3d11.cache_gen_mips)
    {
      extern uint64_t SK_D3D11_MipmapCacheSize;
                      SK_D3D11_MipmapCacheSize += size;

      SK_D3D11_AddDumped  (crc32c,     crc32c   );
      SK_D3D11_AddTexHash (wszOutName, crc32c, 0);
    }

    return ret;
  }

  delete mipmaps;

  return E_FAIL;
}


HRESULT
__stdcall
SK_D3D11_MipmapMakeTexture2D ( ID3D11Device*        pDev,
                               ID3D11DeviceContext* pDevCtx,
                               ID3D11Texture2D*     pInTex,
                               ID3D11Texture2D**   ppOutTex,
                               SK_TLS*              pTLS )
{
  HRESULT                ret     = E_FAIL;
  DirectX::ScratchImage  img;
  DirectX::ScratchImage* mipmaps = nullptr;

  if ( SUCCEEDED (
         DirectX::CaptureTexture ( pDev,   pDevCtx,
                                   pInTex, img     )
                 )
     )
  {
    auto& metadata =
      img.GetMetadata ();

    if ( metadata.width  < 1 ||
         metadata.height < 1    ) return E_INVALIDARG;

    const bool compressed =
      SK_DXGI_IsFormatCompressed (metadata.format);

    try {
      mipmaps =
        new DirectX::ScratchImage;
    }
    catch (...)
    {
      return E_OUTOFMEMORY;
    }

    if (mipmaps != nullptr && compressed )
    {
            DirectX::ScratchImage decompressed;
      const DirectX::Image*       orig_img =
        img.GetImage (0, 0, 0);

      DirectX::TexMetadata meta =
        img.GetMetadata ();

      meta.format    = SK_DXGI_MakeTypedFormat (meta.format);
      meta.mipLevels = 1;

      ret =
        DirectX::Decompress ( orig_img, 1, meta,
                 DXGI_FORMAT_R8G8B8A8_UNORM, decompressed );

      if (SUCCEEDED (ret))
      {
        ret =
          DirectX::GenerateMipMaps ( decompressed.GetImage (0, 0, 0),
                                     1,
                                     decompressed.GetMetadata   (),
                                     DirectX::TEX_FILTER_BOX,
                                     0, *mipmaps );

        if (SUCCEEDED (ret))
        {
          if (! config.textures.d3d11.uncompressed_mips)
          {
            auto* compressed_mips =
              new DirectX::ScratchImage;

            DirectX::TexMetadata mipmap_meta =
              mipmaps->GetMetadata ();

            //
            // Required speedup since BC7 is _slow_
            //
            if (mipmap_meta.format == DXGI_FORMAT_BC7_UNORM)
                mipmap_meta.format = DXGI_FORMAT_BC5_UNORM;

            mipmap_meta.format           =
              SK_DXGI_MakeTypedFormat (mipmap_meta.format);

            const DXGI_FORMAT newFormat =
              SK_DXGI_MakeTypedFormat (img.GetMetadata ().format);

            ret =
              DirectX::Compress ( //This,
                                    mipmaps->GetImages       (),
                                      mipmaps->GetImageCount (),
                                        mipmap_meta,
                                          newFormat,//DXGI_FORMAT_BC7_UNORM,
                                            DirectX::TEX_COMPRESS_DITHER //|
                                            ,//DirectX::TEX_COMPRESS_PARALLEL,
                                              DirectX::TEX_THRESHOLD_DEFAULT, *compressed_mips );

            if (SUCCEEDED (ret))
            {
              delete mipmaps;
                     mipmaps = compressed_mips;
            }

            else
              delete compressed_mips;
          }
        }
      }
    }

    else
    {
      if (mipmaps == nullptr)
          mipmaps = new DirectX::ScratchImage;

      DirectX::TexMetadata meta =
        img.GetMetadata ();

      meta.format = SK_DXGI_MakeTypedFormat (meta.format);

      ret =
        DirectX::GenerateMipMaps ( img.GetImages     (),
                                   img.GetImageCount (),
                                   meta,
                                   DirectX::TEX_FILTER_BOX,
                                     0, *mipmaps );
    }
  }

  if (SUCCEEDED (ret))
  {
    if (mipmaps != nullptr)
    {
      if (ppOutTex != nullptr)
      {
        SK_ScopedBool decl_tex_scope (
          SK_D3D11_DeclareTexInjectScope (pTLS)
        );

        ret =
          DirectX::CreateTexture ( pDev, mipmaps->GetImages     (),
                                         mipmaps->GetImageCount (),
                                         mipmaps->GetMetadata   (),
                                           (ID3D11Resource **)ppOutTex );
      }
    }
  }

  delete mipmaps;

  return ret;
}

HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2D ( _In_ ID3D11Texture2D*      pTex,
                                     uint32_t              crc32c,
                                     SK_TLS*               pTLS,
                                     ID3D11DeviceContext*  pDevCtx,
                                     ID3D11Device*         pDev )
{
  if ( pDev    != nullptr &&
       pDevCtx != nullptr &&
       pTex    != nullptr )
  {
    DirectX::ScratchImage img = { };

    if ( SUCCEEDED (
           DirectX::CaptureTexture ( pDev, pDevCtx,
                                     pTex, img     )
                   )
       )
    {
      auto _IsTypeless = [&](void) ->
      bool { return DirectX::IsTypeless (img.GetMetadata ().format); };

      if (_IsTypeless ())
      {

        img.OverrideFormat   (
          DirectX::MakeTypelessUNORM   (
            img.GetMetadata ( ).format )
                             );

        if (_IsTypeless ())
        {
          img.OverrideFormat   (
            DirectX::MakeTypelessFLOAT   (
              img.GetMetadata ( ).format )
                               );
        }
      }

      return
        SK_D3D11_MipmapCacheTexture2DEx (
          img, crc32c,
            nullptr, nullptr,
                        pTLS
        );
    }
  }

  return E_FAIL;
}


BOOL
SK_D3D11_DeleteDumpedTexture (uint32_t crc32c)
{
  wchar_t  wszPath [ MAX_PATH + 2 ] = { };
  wcscpy ( wszPath,
             SK_EvalEnvironmentVars (
                   SK_D3D11_res_root->c_str ()
                                    ).c_str () );

  lstrcatW (wszPath, L"/dump/textures/");
  lstrcatW (wszPath, SK_GetHostApp ());
  lstrcatW (wszPath, L"/");

  wchar_t    wszOutName [MAX_PATH + 2] = { };
  swprintf ( wszOutName, LR"(%s\Compressed_%08X.dds)",
               wszPath, crc32c );

  uint64_t size =
    SK_File_GetSize (wszOutName);
  if (  DeleteFileW (wszOutName) != FALSE )
  {
    SK_D3D11_RemoveDumped (crc32c, crc32c);

    SK_D3D11_Textures->dumped_texture_bytes -= size;

    return TRUE;
  }

            *wszOutName = L'\0';
  swprintf ( wszOutName, LR"(%s\Uncompressed_%08X.dds)",
               wszPath, crc32c );

  size =
    SK_File_GetSize (wszOutName);
  if (  DeleteFileW (wszOutName) != FALSE )
  {
    SK_D3D11_RemoveDumped (crc32c, crc32c);

    SK_D3D11_Textures->dumped_texture_bytes -= size;

    return TRUE;
  }

  return FALSE;
}

HRESULT
__stdcall
SK_D3D11_DumpTexture2D (  _In_ const D3D11_TEXTURE2D_DESC   *pDesc,
                          _In_ const D3D11_SUBRESOURCE_DATA *pInitialData,
                          _In_       uint32_t                top_crc32,
                          _In_       uint32_t                checksum )
{
  static auto& textures =
    SK_D3D11_Textures;

  SK_LOG0 ( (L"Dumping Texture: %08x::%08x... (fmt=%03lu, "
                    L"BindFlags=0x%04x, Usage=0x%04x, CPUAccessFlags"
                    L"=0x%02x, Misc=0x%02x, MipLODs=%02lu, ArraySize=%02lu)",
                  top_crc32,
                    checksum,
  static_cast <UINT> (pDesc->Format),
                        pDesc->BindFlags,
                          pDesc->Usage,
                            pDesc->CPUAccessFlags,
                              pDesc->MiscFlags,
                                pDesc->MipLevels,
                                  pDesc->ArraySize), L"DX11TexDmp" );

   DXGI_FORMAT newFormat = pDesc->Format;
  if (DirectX::IsTypeless (pDesc->Format))
  { if   (pDesc->Format == DirectX::MakeTypelessUNORM (pDesc->Format))
    { if (pDesc->Format == DirectX::MakeTypelessFLOAT (pDesc->Format))
      {        newFormat = DirectX::MakeSRGB          (pDesc->Format); }
         else{ newFormat = DirectX::MakeTypelessFLOAT (pDesc->Format); } }
         else{ newFormat = DirectX::MakeTypelessUNORM (pDesc->Format); } }

  SK_D3D11_AddDumped (top_crc32, checksum);

  DirectX::TexMetadata mdata;

  mdata.width      =  pDesc->Width;
  mdata.height     =  pDesc->Height;
  mdata.depth      =  1;
  mdata.arraySize  =  pDesc->ArraySize;
  mdata.mipLevels  =  pDesc->MipLevels;
  mdata.miscFlags  =((pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
                                      == D3D11_RESOURCE_MISC_TEXTURECUBE) ?
                                           DirectX::TEX_MISC_TEXTURECUBE  : 0x0;
  mdata.miscFlags2 = 0;
  mdata.format     = newFormat;
  mdata.dimension  = DirectX::TEX_DIMENSION_TEXTURE2D;

  DirectX::ScratchImage image;
  image.Initialize (mdata);

  bool error = false;

  for ( size_t slice = 0;
               slice < mdata.arraySize;
             ++slice )
  {
    size_t     height =
         mdata.height;

    for ( size_t lod = 0;
                 lod < mdata.mipLevels ;
               ++lod )
    {
      const DirectX::Image* img =
        image.GetImage (lod, slice, 0);

      if ( img         == nullptr ||
           img->pixels == nullptr )
      {
        error = true;
        break;
      }

      const size_t lines =
        DirectX::ComputeScanlines (
          mdata.format, height
        );

      if  (lines == 0)
      {
        error = true;
        break;
      }

      auto sptr =
        static_cast <const uint8_t *>(
          pInitialData [lod].pSysMem
        );

      uint8_t* dptr =
        img->pixels;

      for (size_t h = 0; h < lines; ++h)
      {
        const size_t msize =
          std::min <size_t> ( img->rowPitch,
                                pInitialData [lod].SysMemPitch );

        memcpy_s ( dptr, img->rowPitch,
                   sptr, msize );

        sptr += pInitialData [lod].SysMemPitch;
        dptr += img->rowPitch;
      }

      if (height > 1)
          height >>= 1;
    }

    if (error)
      break;
  }

  wchar_t  wszPath [ MAX_PATH + 2 ] = { };
  wcscpy ( wszPath,
             SK_EvalEnvironmentVars (
                   SK_D3D11_res_root->c_str ()
                                    ).c_str () );

  lstrcatW             (wszPath, L"/dump/textures/");
  lstrcatW             (wszPath, SK_GetHostApp ());
  lstrcatW             (wszPath, L"/");
  SK_CreateDirectories (wszPath);

  const bool compressed =
    SK_DXGI_IsFormatCompressed (pDesc->Format);

  wchar_t wszOutPath [MAX_PATH + 2] = { };
  wchar_t wszOutName [MAX_PATH + 2] = { };

  wcscpy ( wszOutPath,
             SK_EvalEnvironmentVars (
                   SK_D3D11_res_root->c_str ()
                                    ).c_str () );

  lstrcatW (wszOutPath, LR"(\dump\textures\)");
  lstrcatW (wszOutPath, SK_GetHostApp ());

  if (compressed && config.textures.d3d11.precise_hash) {
    swprintf ( wszOutName, LR"(%s\Compressed_%08X_%08X.dds)",
                 wszOutPath, top_crc32, checksum );
  } else if (compressed) {
    swprintf ( wszOutName, LR"(%s\Compressed_%08X.dds)",
                 wszOutPath, top_crc32 );
  } else if (config.textures.d3d11.precise_hash) {
    swprintf ( wszOutName, LR"(%s\Uncompressed_%08X_%08X.dds)",
                 wszOutPath, top_crc32, checksum );
  } else {
    swprintf ( wszOutName, LR"(%s\Uncompressed_%08X.dds)",
                 wszOutPath, top_crc32 );
  }

  if ((! error) && wszOutName [0] != L'\0')
  {
    if (GetFileAttributes (wszOutName) == INVALID_FILE_ATTRIBUTES)
    {
      SK_LOG0 ( (L" >> File: '%s' (top: %x, full: %x)",
                      wszOutName,
                        top_crc32,
                          checksum), L"DX11TexDmp" );

      wchar_t   wszMetaFilename [ MAX_PATH + 2 ] = { };
      swprintf (wszMetaFilename, L"%s.txt", wszOutName);

      FILE* fMetaData = _wfopen (wszMetaFilename, L"w+");

      if (fMetaData != nullptr) {
        fprintf ( fMetaData,
                  "Dumped Name:    %ws\n"
                  "Texture:        %08x::%08x\n"
                  "Dimensions:     (%ux%u)\n"
                  "Format:         %03u (%hs)\n"
                  "MipLODs:        %02u\n"
                  "----------------\n"
                  "BindFlags:      0x%04x\n"
                  "Usage:          0x%04x\n"
                  "CPUAccessFlags: 0x%02x\n"
                  "Misc:           0x%02x\n"
                  "ArraySize:      %02u",
                  wszOutName,
                    top_crc32,
                      checksum,
                        pDesc->Width, pDesc->Height,
                        pDesc->Format, SK_DXGI_FormatToStr (pDesc->Format).data (),
                          pDesc->MipLevels,
                            pDesc->BindFlags,
                              pDesc->Usage,
                                pDesc->CPUAccessFlags,
                                  pDesc->MiscFlags,
                                    pDesc->ArraySize );

        fclose (fMetaData);
      }

      const HRESULT hr =
        SaveToDDSFile ( image.GetImages (), image.GetImageCount (),
                          image.GetMetadata (), DirectX::DDS_FLAGS_NONE,
                            wszOutName );

      if (SUCCEEDED (hr))
      {
        textures->dumped_texture_bytes +=
          SK_File_GetSize (wszOutName);
      }
    }
  }

  return E_FAIL;
}


extern void
SK_D3D11_ProcessScreenshotQueueEx (
  SK_ScreenshotStage stage_ = SK_ScreenshotStage::EndOfFrame,
  bool                 wait = false,
  bool                purge = false );


SK_LazyGlobal <SK_D3D11_TexMgr> SK_D3D11_Textures;


class SK_D3D11_TexCacheMgr {
public:
};


std::string
SK_D3D11_SummarizeTexCache (void)
{
  char szOut [512] { };

  static auto& textures =
    SK_D3D11_Textures;

  snprintf ( szOut, 511, "  Tex Cache: %#5llu MiB   Entries:   %#7u\n"
                         "  Hits:      %#5u       Time Saved: %#7.01lf ms\n"
                         "  Evictions: %#5u",
               textures->AggregateSize_2D >> 20ULL,
               textures->Entries_2D.load        (),
               textures->RedundantLoads_2D.load (),
               textures->RedundantTime_2D,
               textures->Evicted_2D.load        () );

  return
    std::move (szOut);
}


bool
SK_D3D11_TexMgr::isTexture2D ( uint32_t              crc32,
                         const D3D11_TEXTURE2D_DESC *pDesc )
{
  if (! (SK_D3D11_cache_textures && crc32))
  {
    return false;
  }

  return
    HashMap_2D [pDesc->MipLevels].contains (crc32);
}

void
__stdcall
SK_D3D11_ResetTexCache (void)
{
  SK_D3D11_ProcessScreenshotQueueEx (
    SK_ScreenshotStage::_FlushQueue, false, true
  );

//  SK_D3D11_need_tex_reset = true;
  SK_D3D11_try_tex_reset = true;
  SK_D3D11_Textures->reset ();
}


volatile ULONG SK_D3D11_LiveTexturesDirty = FALSE;

// Call this from the OSD's immediate context thread and nowhere else
void
__stdcall
SK_D3D11_TexCacheCheckpoint (void)
{
  static auto& textures =
    SK_D3D11_Textures;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  static auto& Entries_2D  = textures->Entries_2D;
  static auto& Evicted_2D  = textures->Evicted_2D;
  static auto& Textures_2D = textures->Textures_2D;

  auto pDevice =
    rb.getDevice <ID3D11Device> ();
  SK_ComQIPtr    <IDXGIDevice>  pDXGIDev (pDevice);

  if ( config.textures.cache.residency_managemnt &&
       pDevice.p != nullptr                      &&
       pDXGIDev  != nullptr )
  {
    const
    auto   MAX_TEXTURES_PER_PASS  = 32UL;

    static DWORD  dwLastEvict     = 0;
    static DWORD  dwLastTest      = 0;
    static DWORD  dwLastSize      = Entries_2D.load ();

    static DWORD  dwInitiateEvict = 0;
    static DWORD  dwInitiateSize  = 0;

    const  DWORD  dwNow           = SK_timeGetTime ();

    static size_t cur_tex         = 0;

    if ( ( Evicted_2D.load () != dwLastEvict ||
           Entries_2D.load () != dwLastSize     ) &&
                    dwLastTest < dwNow - 666L )
    {
      if (cur_tex == 0)
      {
        dwInitiateEvict = Evicted_2D.load ();
        dwInitiateSize  = Entries_2D.load ();
      }

      static LONG fully_resident = 0;
      static LONG shared_memory  = 0;
      static LONG on_disk        = 0;

      static LONG64 size_vram   = 0ULL;
      static LONG64 size_shared = 0ULL;
      static LONG64 size_disk   = 0ULL;

      static std::array < IUnknown *,
                          MAX_TEXTURES_PER_PASS + 2 > residency_tests;
      static std::array < SK_D3D11_TexMgr::tex2D_descriptor_s*,
                          MAX_TEXTURES_PER_PASS + 2 > residency_descs;
      static std::array < DXGI_RESIDENCY,
                          MAX_TEXTURES_PER_PASS + 2>  residency_results;

      size_t record_count = 0;

      auto record =
        residency_tests.begin ();
      auto desc =
        residency_descs.begin ();

      const size_t start_idx = cur_tex;
            size_t idx       = 0;
      const size_t max_size  = Textures_2D.size ();

      bool done = false;

      for ( auto& tex : Textures_2D )
      {
        if (++idx < cur_tex)
          continue;

        if (idx > start_idx + MAX_TEXTURES_PER_PASS)
        {
          cur_tex = idx;
          break;
        }

        if (tex.second.crc32c != 0x0 && tex.first != nullptr)
        {
          *(record++) =  tex.first;
          *(desc++)   = &tex.second;
                      ++record_count;
        }
      }

      if (idx >= max_size)
        done = true;

      pDXGIDev->QueryResourceResidency (
        residency_tests.data   (),
        residency_results.data (),
          static_cast <UINT> (
            record_count
          )
      );

      idx  = 0;
      desc = residency_descs.begin ();

      for ( auto& it : residency_results )
      {
        // Handle uninit. and corrupted texture entries
        if ( static_cast     <int> (it)           <
                 static_cast <int> (DXGI_RESIDENCY_FULLY_RESIDENT) ||
             static_cast     <int> (it)           >
                 static_cast <int> (DXGI_RESIDENCY_EVICTED_TO_DISK) )
        {
          continue;
        }

        SK_LOG1 ( ( L"Texture %x is non-resident, last use: %lu "
                    L"<Residence: %lu>",
                      (*desc)->crc32c,
                      (*desc)->last_used, it ),
                   L"DXGI Cache" );

        const D3D11_TEXTURE2D_DESC* tex_desc =
                             &(*desc)->desc;

        if ((*desc)->texture != nullptr)
        {
          const UINT refs_plus_1 = (*desc)->texture->AddRef  ();
          const UINT refs        = (*desc)->texture->Release ();

          SK_LOG1 (
            ( L"(%lux%lu@%lu [%s] - %s, %s, %s : CPU Usage=%x -- "
              L"refs+1=%lu, refs=%lu",
              tex_desc->Width,
              tex_desc->Height,           tex_desc->MipLevels,
              SK_DXGI_FormatToStr        (tex_desc->Format).   data  (),
              SK_D3D11_DescribeBindFlags (tex_desc->BindFlags).c_str (),
              SK_D3D11_DescribeMiscFlags (
                (D3D11_RESOURCE_MISC_FLAG)tex_desc->MiscFlags).c_str (),
              SK_D3D11_DescribeUsage     (tex_desc->Usage),
                                    (UINT)tex_desc->CPUAccessFlags,
                  refs_plus_1, refs ),
               L"DXGI Cache" );

          // If this texture is _NOT_ injected and also not resident in VRAM,
          //   then remove it from cache.
          //
          //  If it is injected, leave it loaded because this cache's purpose
          //    is to prevent re-loading injected textures.
          if (refs == 1 && (! (*desc)->injected))
            (*desc)->texture->Release ();
        }

        if (it == DXGI_RESIDENCY_FULLY_RESIDENT)
          { ++fully_resident; size_vram   += (*(desc))->mem_size; }
        if (it == DXGI_RESIDENCY_RESIDENT_IN_SHARED_MEMORY)
          { ++shared_memory;  size_shared += (*(desc))->mem_size; }
        if (it == DXGI_RESIDENCY_EVICTED_TO_DISK)
          { ++on_disk;        size_disk   += (*(desc))->mem_size; }

        ++desc;

        if (++idx >= record_count)
          break;
      }

      dwLastTest = dwNow;

      if (done)
      {
        auto& count = SK_D3D11_TexCacheResidency->count;
        auto& size  = SK_D3D11_TexCacheResidency->size;

        InterlockedExchange   (&count.InVRAM,   fully_resident);
        InterlockedExchange   (&count.Shared,   shared_memory);
        InterlockedExchange   (&count.PagedOut, on_disk);

        InterlockedExchange64 (&size.InVRAM,   size_vram);
        InterlockedExchange64 (&size.Shared,   size_shared);
        InterlockedExchange64 (&size.PagedOut, size_disk);

        textures->AggregateSize_2D = size_vram      + size_shared + size_disk;
        textures->Entries_2D       = fully_resident + shared_memory + on_disk;

        fully_resident = 0;
        shared_memory  = 0;
        on_disk        = 0;

        size_vram   = 0ULL;
        size_shared = 0ULL;
        size_disk   = 0ULL;

        dwLastEvict = dwInitiateEvict;
        dwLastSize  = dwInitiateSize;

        cur_tex = 0;
      }
    }
  }



         int       iter               = 0;

  static bool      init               = false;
  static ULONGLONG ullMemoryTotal_KiB = 0;
  static HANDLE    hProc              = nullptr;

  if (! init)
  {
    init  = true;
    hProc = SK_GetCurrentProcess ();

    GetPhysicallyInstalledSystemMemory (&ullMemoryTotal_KiB);
  }

  ++iter;

  bool reset =
    SK_D3D11_need_tex_reset || SK_D3D11_try_tex_reset;

  PROCESS_MEMORY_COUNTERS pmc = {   };

  const bool has_non_zero_reserve =
    config.mem.reserve > 0.0f;

  if ((iter % 5) == 0)
  {
                                  pmc.cb = sizeof pmc;
    GetProcessMemoryInfo (hProc, &pmc,     sizeof pmc);

    reset |=
      (          (textures->AggregateSize_2D >> 20ULL) > (uint64_t)cache_opts.max_size    ||
                  textures->Entries_2D                 >           cache_opts.max_entries ||
                                                                                    false
     //( has_non_zero_reserve && ((config.mem.reserve / 100.0f) * ullMemoryTotal_KiB)
     //                                               < (pmc.PagefileUsage >> 10UL)
     //)
     ) ;
  }

  if (reset)
  {
    //dll_log->Log (L"[DX11TexMgr] DXGI 1.4 Budget Change: Triggered a texture manager purge...");

    SK_D3D11_amount_to_purge =
      has_non_zero_reserve   ?
      static_cast <int32_t> (
        (pmc.PagefileUsage >> 10UL) - (float)(ullMemoryTotal_KiB) *
                       (config.mem.reserve / 100.0f)
      )                      : 0;

    textures->reset ();
  }

  else
  {
    //SK_RenderBackend& rb =
    //  SK_GetCurrentRenderBackend ();

    //if (rb.d3d11.immediate_ctx != nullptr)
    //{
    //  SK_D3D11_PreLoadTextures ();
    //}
  }
}

void
SK_D3D11_TexMgr::reset (void)
{
  // We have nothing to reset if these hooks aren't established
  if ( IUnknown_AddRef_Original  == nullptr ||
       IUnknown_Release_Original == nullptr )
  {
    return;
  }


  if (SK_GetFramesDrawn () < 1)
    return;


  uint32_t count  = 0;
  int64_t  purged = 0;


  bool must_free =
    SK_D3D11_need_tex_reset;

  SK_D3D11_need_tex_reset = false;


  const int64_t time_now =
    SK_QueryPerf ().QuadPart;

  // Additional conditions that may trigger a purge
  if (! must_free)
  {
    // Throttle to at most one potentially unnecessary purge attempt per-ten seconds
    if ( LastModified_2D <=  LastPurge_2D &&
         time_now        < ( LastPurge_2D + ( SK_QpcFreq * 10LL ) ) )
    {
      SK_D3D11_try_tex_reset = true;
      return;
    }


    constexpr float overfill_factor = 1.05f;

    bool no_work = true;


    if ( (uint32_t)AggregateSize_2D >> 20ULL > ( (uint32_t)       cache_opts.max_size +
                                                ((uint32_t)(float)cache_opts.max_size * overfill_factor)) )
      no_work = false;

    if ( SK_D3D11_Textures->Entries_2D > (LONG)       cache_opts.max_entries +
                                         (LONG)(float)cache_opts.max_entries * overfill_factor )
      no_work = false;

    if ( SK_D3D11_amount_to_purge > 0 )
      no_work = false;

    if (no_work) return;
  }


  std::set    <ID3D11Texture2D *>                     cleared;// (cache_opts.max_evict);
  std::vector <SK_D3D11_TexMgr::tex2D_descriptor_s *> textures;

  int potential = 0;
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> critical (*cache_cs);
    {
      textures.reserve  (Textures_2D.size ());

      for ( auto& desc : Textures_2D )
      {
        if (desc.second.texture == nullptr || desc.second.crc32c == 0x00)
          continue;

        bool can_free = must_free;

        if (! must_free)
          can_free = (IUnknown_AddRef_Original (desc.second.texture) <= 2);

        if (can_free)
        {
          ++potential;
            textures.emplace_back (&desc.second);
        }

        if (! must_free)
          IUnknown_Release_Original (desc.second.texture);
      }
    }

    if (potential > 0)
    {
      std::sort ( std::execution::par,
                    textures.begin (),
                    textures.end   (),
         [&]( const SK_D3D11_TexMgr::tex2D_descriptor_s* const a,
              const SK_D3D11_TexMgr::tex2D_descriptor_s* const b )
        {
          return ( b->load_time * 10ULL  +  b->last_used / 10ULL  +
                    static_cast <uint64_t> (b->hits)     * 100ULL   )   <
                 ( a->load_time * 10ULL  +  a->last_used / 10ULL  +
                    static_cast <uint64_t> (a->hits)     * 100ULL   );
        }
      );
    }
  }

  if (potential == 0)
    return;

  dll_log->Log (L"Reset potential = %lu / %lu", potential, textures.size ());

  const uint32_t max_count =
   cache_opts.max_evict;

  std::scoped_lock <SK_Thread_HybridSpinlock> critical (*cache_cs);

  for ( const auto& desc : textures )
  {
    const auto mem_size =
     sk::narrow_cast <int64_t> (desc->mem_size) >> 10ULL;

   if ( desc->texture != nullptr && cleared.count (desc->texture) == 0 )
   {
     const int refs =
       desc->texture->AddRef  () - 1;
       desc->texture->Release ();

     if (refs <= 1 || must_free)
     {
       // Avoid double-freeing if the hash map somehow contains the same
       // texture multiple times
       if (! cleared.emplace (desc->texture).second)
       {
         SK_LOG0 ( ( L"Encountered cached texture with crc32="
                       L"%08x in multiple cache descriptors; refs=%lu",
                         desc->crc32c, refs ),
                      L"DX11TexMgr" );
       }

       if (must_free && refs != 1)
       {
         SK_LOG0 ( ( L"Unexpected reference count for texture with crc32="
                     L"%08x; refs=%lu, expected=1 -- removing from cache "
                     L"and praying...",
                       desc->crc32c, refs ),
                    L"DX11TexMgr" );
       }

       SK_D3D11_RemoveTexFromCache (desc->texture);
         IUnknown_Release_Original (desc->texture);

       count++;
       purged += mem_size;

       if ( ! must_free )
       {
         if ((// Have we over-freed? If so, stop when the minimum number of
              // evicted textures is reached
              (AggregateSize_2D >> 20ULL) <= (uint32_t)cache_opts.min_size &&
                                    count >=           cache_opts.min_evict )

              // An arbitrary purge request was issued
                                                                  ||
             (SK_D3D11_amount_to_purge     >  0                   &&
              SK_D3D11_amount_to_purge     <= purged              &&
                                     count >= cache_opts.min_evict ) ||

              // Have we evicted as many textures as we can in one pass?
                                     count >= max_count )
         {
           SK_D3D11_amount_to_purge =
             std::max (0, SK_D3D11_amount_to_purge);

           break;
         }
       }
     }
   }
 }

 LastPurge_2D.store (time_now);

 if (count > 0)
 {
   SK_D3D11_try_tex_reset = false;

   SK_LOG0 ( ( L"Texture Cache Purge:  Eliminated %.2f MiB of texture data"
               L" across %lu textures (max. evict per-pass: %lu).",
                 static_cast <float> (purged) / 1024.0f,
                                      count,
                                      cache_opts.max_evict ),
               L"DX11TexMgr" );

   if ((AggregateSize_2D >> 20ULL) >= static_cast <uint64_t> (cache_opts.max_size))
   {
     SK_LOG0 ( ( L" >> Texture cache remains %.2f MiB over-budget; will "
                 L"schedule future passes until resolved.",
                   static_cast <double> ( AggregateSize_2D ) / (1024.0 * 1024.0) -
                   static_cast <double> (cache_opts.max_size) ),
                 L"DX11TexMgr" );

     SK_D3D11_try_tex_reset = true;
   }

   if (Entries_2D >= cache_opts.max_entries)
   {
     SK_LOG0 ( ( L" >> Texture cache remains %lu entries over-filled; will"
                 L" schedule future passes until resolved.",
                   Entries_2D - cache_opts.max_entries ),
                 L"DX11TexMgr" );

     SK_D3D11_try_tex_reset = true;
   }

   InterlockedExchange (&SK_D3D11_LiveTexturesDirty, TRUE);
 }
}

LONG
SK_D3D11_TexMgr::recordCacheHit ( ID3D11Texture2D *pTex )
{
  static auto& textures =
    SK_D3D11_Textures;

  const auto& tex2D =
    textures->Textures_2D.find (pTex);

  if ( tex2D                != textures->Textures_2D.cend () &&
       tex2D->second.crc32c != 0x00 )
  {
    auto& desc2d =
      tex2D->second;

    SK_TLS *pTLS =
      SK_TLS_Bottom ();

    // Don't record cache hits caused by the shader mod interface
    ////////if (pTLS == nullptr || pTLS->imgui->drawing == FALSE)
    if (! (pTLS && pTLS->imgui->drawing ) )
    {
      desc2d.last_used =
        SK_QueryPerf ().QuadPart;

      return
        InterlockedIncrement (&desc2d.hits);
    }
  }

  return 0;
}

ID3D11Texture2D*
SK_D3D11_TexMgr::getTexture2D ( uint32_t               tag,
                          const D3D11_TEXTURE2D_DESC  *pDesc,
                                size_t                *pMemSize,
                                float                 *pTimeSaved,
                                SK_TLS                *pTLS )
{
  ID3D11Texture2D* pTex2D =
    nullptr;

  if (isTexture2D (tag, pDesc))
  {
    ID3D11Texture2D*    it =     HashMap_2D [pDesc->MipLevels][tag];
    tex2D_descriptor_s& desc2d (Textures_2D [it]);

    // We use a lockless concurrent hashmap, which makes removal
    //   of key/values an unsafe operation.
    //
    //   Thus, crc32c == 0x0 signals a key that exists in the
    //     map, but has no live association with any real data.
    //
    //     Zombie, in other words.
    //
    const bool zombie =
      ( desc2d.crc32c == 0x00 );

    if ( (! zombie)            &&
            desc2d.tag == tag  &&
         (! desc2d.discard) )
    {
      pTex2D =
            desc2d.texture;
    }

    if (pTex2D != nullptr)
    {
      pTex2D->AddRef ();

      const size_t  size = desc2d.mem_size;
      const float  fTime = static_cast <float> (desc2d.load_time ) * 1000.0f /
                           static_cast <float> (SK_QpcFreq);

      if (pMemSize   != nullptr) *pMemSize   = size;
      if (pTimeSaved != nullptr) *pTimeSaved = fTime;

      desc2d.last_used =
        SK_QueryPerf ().QuadPart;

      if (pTLS == nullptr)
          pTLS = SK_TLS_Bottom ();

      // Don't record cache hits caused by the shader mod interface
      //if (! SK_ImGui_IsDrawing_OnD3D11Ctx (UINT_MAX, nullptr))
      if (pTLS->imgui->drawing == FALSE)
      {
        InterlockedIncrement (&desc2d.hits);

        RedundantData_2D += sk::narrow_cast <LONG64> (size);
        RedundantLoads_2D++;
        RedundantTime_2D += fTime;
      }
    }


    else
    {
      if (! zombie)
      {
        SK_LOG0 ( ( L"Cached texture (tag=%x) found in hash table, but not"
                    L" in texture map", tag),
                    L"DX11TexMgr" );
      }

      HashMap_2D   [pDesc->MipLevels].erase (
        HashMap_2D [pDesc->MipLevels][tag]  );
      HashMap_2D   [pDesc->MipLevels].erase (
                                      tag   );
      Textures_2D  [it].crc32c = 0x00;
    }
  }

  return pTex2D;
}

bool
SK_D3D11_SafeGetTexDesc (ID3D11Texture2D* pTex, D3D11_TEXTURE2D_DESC* desc)
{
  __try
  {
    if (pTex != nullptr)
    {
      pTex->GetDesc (desc);

      return true;
    }
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                    EXCEPTION_EXECUTE_HANDLER  :
                                    EXCEPTION_CONTINUE_SEARCH  )
  { };

  return false;
}

bool
__stdcall
SK_D3D11_TextureIsCachedEx (ID3D11Texture2D* pTex, bool touch = false)
{
  if (! (SK_D3D11_cache_textures && pTex != nullptr))
  {
    return
      false;
  }

  static auto& textures =
    SK_D3D11_Textures;

  D3D11_TEXTURE2D_DESC                 tex_desc           =  { };
  if ( SK_D3D11_SafeGetTexDesc (pTex, &tex_desc)   &&
                                       tex_desc.MipLevels <  20 )
  {
    if (textures->HashMap_2D [tex_desc.MipLevels].contains (pTex))
    {
      if (touch && (! SK_D3D11_IsTexInjectThread ()))
      {
        textures->HashMap_2D [tex_desc.MipLevels].touch (pTex);
      }

      return
        true;
    }
  }

  return
    false;
}

bool
__stdcall
SK_D3D11_TextureIsCached (ID3D11Texture2D* pTex)
{
  return
    SK_D3D11_TextureIsCachedEx (pTex);
}

uint32_t
__stdcall
SK_D3D11_TextureHashFromCache (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return 0x00;

  static auto& textures =
    SK_D3D11_Textures;

  const auto& it =
    textures->Textures_2D.find (pTex);

  if ( it != textures->Textures_2D.cend () )
    return it->second.crc32c;

  return 0x00;
}

BOOL
SK_D3D11_MarkTextureUncacheable ( ID3D11Texture2D *pTexture )
{
  constexpr UINT size =
                 sizeof (LONG);

  static LONG disable = 1;

  if ( FAILED (
         pTexture->SetPrivateData ( SKID_D3D11TextureUncacheable,
                                      size, &disable )
       )
     )
  {
    return FALSE;
  }

  return TRUE;
}

BOOL
SK_D3D11_IsTextureUncacheable ( ID3D11Texture2D *pTexture )
{
  UINT size =
       sizeof (LONG);

  LONG disable = 0;

  if ( FAILED (
         pTexture->GetPrivateData ( SKID_D3D11TextureUncacheable,
                                     &size, (void *)&disable )
       )
     )
  {
    return FALSE;
  }

  return
    (disable == 1);
}

void
__stdcall
SK_D3D11_UseTexture (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return;

  // If cached, touch the last access time
  SK_D3D11_TextureIsCachedEx (pTex, true);
}

bool
__stdcall
SK_D3D11_RemoveTexFromCache (ID3D11Texture2D* pTex, bool blacklist)
{
  if (! SK_D3D11_TextureIsCached (pTex))
    return false;

  static auto& textures =
    SK_D3D11_Textures;

  if (pTex != nullptr)
  {
    SK_D3D11_TexMgr::tex2D_descriptor_s& it =
      textures->Textures_2D [pTex];

          uint32_t              tag  = it.tag;
    const D3D11_TEXTURE2D_DESC& desc = it.orig_desc;

    textures->AggregateSize_2D -=
      it.mem_size;
      it.crc32c = 0x00;

  //SK_D3D11_Textures->Textures_2D.erase (pTex);
    textures->Evicted_2D++;
  //SK_D3D11_Textures->TexRefs_2D.erase (pTex);

    textures->Entries_2D--;

    if (blacklist)
    {
      if (textures->Blacklist_2D [desc.MipLevels].emplace (tag).second)
        pTex->Release (); // First removal needs our reference stripped,
                          //   all others do not
    }
    else
    {
      textures->HashMap_2D [desc.MipLevels].erase ( tag);
      textures->HashMap_2D [desc.MipLevels].erase (pTex);
    }

    InterlockedExchange (&SK_D3D11_LiveTexturesDirty, TRUE);
  }

  // Lightweight signal that something changed and a purge may be needed
  textures->LastModified_2D = SK_QueryPerf ().QuadPart;

  return true;
}

void
SK_D3D11_TexMgr::updateDebugNames (void)
{
  for (auto it : HashMap_2D)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> _lock (*(it.mutex));

    for (auto it2 : it.entries)
    {
      if (it2.second == nullptr)
        continue;

      if ( TexRefs_2D.count  (it2.second) &&
           Textures_2D.count (it2.second) )
      {
        const auto& tex_ref =
          TexRefs_2D.find (it2.second);

        auto& tex_desc =
          Textures_2D [*tex_ref];

        if (tex_desc.debug_name.empty ())
        {
          char     szDesc [128] = { };
          wchar_t wszDesc [128] = { };
          UINT uiDescLen = 127;

          auto se_orig =
          SK_SEH_ApplyTranslator (
            SK_FilteringStructuredExceptionTranslator (
              EXCEPTION_ACCESS_VIOLATION
            )
          );
          try
          {
            uiDescLen = sizeof (wszDesc) - sizeof (wchar_t);

            if ( SUCCEEDED (
                 (*tex_ref)->GetPrivateData (
                   WKPDID_D3DDebugObjectNameW,
                         &uiDescLen, wszDesc )
                           ) && uiDescLen > sizeof (wchar_t)
               )
            {
              tex_desc.debug_name =
                SK_WideCharToUTF8 (wszDesc);
            }

            else
            {
              uiDescLen = sizeof (szDesc) - sizeof (char);

              if ( SUCCEEDED (
                     (*tex_ref)->GetPrivateData (
                       WKPDID_D3DDebugObjectName,
                             &uiDescLen, szDesc )
                             ) && uiDescLen > sizeof (char)
                 )
              {
                tex_desc.debug_name =
                  szDesc;
              }
            }
          }
          catch (const SK_SEH_IgnoredException&) { }
          SK_SEH_RemoveTranslator (se_orig);
        }
      }
    }
  }
}

void
SK_D3D11_TexMgr::refTexture2D ( ID3D11Texture2D*      pTex,
                          const D3D11_TEXTURE2D_DESC *pDesc,
                                uint32_t              tag,
                                size_t                mem_size,
                                uint64_t              load_time,
                                uint32_t              crc32c,
                          const wchar_t              *fileName,
                          const D3D11_TEXTURE2D_DESC *pOrigDesc,
                       _In_opt_ HMODULE              /*hModCaller*/,
                       _In_opt_ SK_TLS               *pTLS )
{
  if (! SK_D3D11_cache_textures)
    return;

  if (pTex == nullptr || tag == 0x00)
    return;


  static volatile LONG init = FALSE;

  if (FALSE == InterlockedCompareExchangeAcquire (&init, TRUE, FALSE))
  {
    DXGI_VIRTUAL_HOOK ( &pTex, 2, "IUnknown::Release",
                        IUnknown_Release,
                        IUnknown_Release_Original,
                        IUnknown_Release_pfn );
    DXGI_VIRTUAL_HOOK ( &pTex, 1, "IUnknown::AddRef",
                        IUnknown_AddRef,
                        IUnknown_AddRef_Original,
                        IUnknown_AddRef_pfn );

    SK_ApplyQueuedHooks ();

    InterlockedIncrementRelease (&init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&init, 2);



  if (SK_D3D11_TestRefCountHooks (pTex, pTLS) != FALSE)
  {
    SK_LOG2 ( (L"Cached texture (%x)",
                  crc32c ),
               L"DX11TexMgr" );
  }

  else
  {
    SK_LOG0 ( (L"Potentially cacheable texture (%x) is not correctly "
               L"reference counted; opting out!", crc32c ),
               L"DX11TexMgr" );
    return;
  }


  ///if (pDesc->Usage >= D3D11_USAGE_DYNAMIC)
  ///{
  ///  dll_log->Log ( L"[DX11TexMgr] Texture '%08X' Is Not Cacheable "
  ///                 L"Due To Usage: %lu",
  ///                 crc32c, pDesc->Usage );
  ///  return;
  ///}

  ///if (pDesc->CPUAccessFlags & D3D11_CPU_ACCESS_WRITE)
  ///{
  ///  dll_log->Log ( L"[DX11TexMgr] Texture '%08X' Is Not Cacheable "
  ///                 L"Due To CPUAccessFlags: 0x%X",
  ///                 crc32c, pDesc->CPUAccessFlags );
  ///  return;
  ///}

  static
    tex2D_descriptor_s  null_desc = { };
    tex2D_descriptor_s&   texDesc = null_desc;

  if ( SK_D3D11_TextureIsCached (pTex) &&
      (! (texDesc = Textures_2D [pTex]).discard) )
  {
    // If we are updating once per-frame, then remove the freaking texture :)
    if (HashMap_2D [texDesc.orig_desc.MipLevels].contains (texDesc.tag))
    {
      if (texDesc.last_frame > SK_GetFramesDrawn () - 3)
      {
        texDesc.discard = true;

        pTex->SetPrivateData (
          SKID_D3D11Texture2D_DISCARD,
            sizeof (bool),
              &texDesc.discard
        );
      }

      //// This texture is too dynamic, it's best to just cut all ties to it,
      ////   don't try to keep updating the hash because that may kill performance.
      ////////HashMap_2D [texDesc.orig_desc.MipLevels].erase (texDesc.tag);
    }

    texDesc.crc32c     = crc32c;
    texDesc.tag        = tag;
    texDesc.last_used  = SK_QueryPerf      ().QuadPart;
    texDesc.last_frame = SK_GetFramesDrawn ();

    ///InterlockedIncrement (&texDesc.hits);

    dll_log->Log ( L"[DX11TexMgr] Texture is already cached?!  "
                   L"{ Original: %08x, New: %08x }",
                     Textures_2D [pTex].crc32c,
                                        crc32c );
  }

  if (texDesc.discard)
  {
    //dll_log->Log ( L"[DX11TexMgr] Texture was cached, but marked as "
    //               L"discard... ignoring" );
    return;
  }


  AggregateSize_2D += mem_size;

  tex2D_descriptor_s desc2d
                    = { };

  desc2d.desc       = *pDesc;
  desc2d.orig_desc  =  pOrigDesc != nullptr ?
                      *pOrigDesc : *pDesc;
  desc2d.texture    =  pTex;
  desc2d.load_time  =  load_time;
  desc2d.mem_size   =  mem_size;
  desc2d.tag        =  tag;
  desc2d.crc32c     =  crc32c;
  desc2d.last_used  =  SK_QueryPerf      ().QuadPart;
  desc2d.last_frame =  SK_GetFramesDrawn ();
  desc2d.file_name  =  fileName;

  pTex->SetPrivateData (
    SKID_D3D11Texture2D_BaseObj,
      sizeof (LPVOID),
        &desc2d.texture );


  if (desc2d.orig_desc.MipLevels >= 18)
  {
    SK_LOG0 ( ( L"Too many mipmap LODs to cache (%lu), will not cache '%x'",
                  desc2d.orig_desc.MipLevels,
                    desc2d.crc32c ),
                L"DX11TexMgr" );
  }

  SK_LOG4 ( ( L"Referencing Texture '%x' with %lu mipmap levels :: (%08" _L(PRIxPTR) L"h)",
                desc2d.crc32c,
                  desc2d.orig_desc.MipLevels,
                    (uintptr_t)pTex ),
              L"DX11TexMgr" );

  SK_LOG4 ( ( L"  >> (%ux%u:%u) [CPU Access: %x], Misc Flags: %x, Usage: %u, Bind Flags: %x",
                desc2d.orig_desc.Width,     desc2d.orig_desc.Height,
                desc2d.orig_desc.MipLevels, desc2d.orig_desc.CPUAccessFlags,
                desc2d.orig_desc.MiscFlags, desc2d.orig_desc.Usage,
                desc2d.orig_desc.BindFlags ),
              L"DX11TexMgr" );

  // Hold a reference ourselves so that the game cannot free it
  pTex->AddRef ();

  HashMap_2D [desc2d.orig_desc.MipLevels][tag]  = pTex;
  HashMap_2D [desc2d.orig_desc.MipLevels][pTex] = tag;

  // Also insert into the hashmap for the number of mipmaps in the MODIFIED texture
  if (desc2d.orig_desc.MipLevels != desc2d.desc.MipLevels)
  {
    HashMap_2D [desc2d.desc.MipLevels][tag]  = pTex;
    HashMap_2D [desc2d.desc.MipLevels][pTex] = tag;
  }

  Textures_2D.insert            (std::make_pair (pTex, desc2d));
  TexRefs_2D.insert             (                pTex);

  Entries_2D++;


  // Lightweight signal that something changed and a purge may be needed
  LastModified_2D = SK_QueryPerf ().QuadPart;
}


SK_LazyGlobal <
    std::unordered_map < std::wstring, uint32_t >
  >      SK_D3D11_EnumeratedMipmapCache;
uint64_t SK_D3D11_MipmapCacheSize;

void
SK_D3D11_RecursiveEnumAndAddTex ( std::wstring   directory,
                                  unsigned int  &files,
                                  LARGE_INTEGER &liSize,
                                  wchar_t       *wszPattern =
                                                   (wchar_t *)L"*" );

void
WINAPI
SK_D3D11_PopulateResourceList (bool refresh)
{
  static bool init = false;

  if (((! refresh) && init) || SK_D3D11_res_root->empty ())
    return;

  std::scoped_lock < SK_Thread_HybridSpinlock,
                     SK_Thread_HybridSpinlock >
       double_lock (
         *tex_cs,
         *inject_cs
       );

  static auto& textures =
    SK_D3D11_Textures;

  if (refresh)
  {
    textures->dumped_textures.clear ();
    textures->dumped_texture_bytes = 0;

    textures->injectable_textures.clear ();
    textures->injectable_texture_bytes = 0;

    SK_D3D11_EnumeratedMipmapCache->clear ();
               SK_D3D11_MipmapCacheSize = 0;
  }

  init = true;

  wchar_t wszTexDumpDir_RAW [ MAX_PATH + 2 ] = { };
  wchar_t wszTexDumpDir     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexDumpDir_RAW, SK_D3D11_res_root->c_str ());
  lstrcatW (wszTexDumpDir_RAW, LR"(\dump\textures\)");
  lstrcatW (wszTexDumpDir_RAW, SK_GetHostApp ());

  wcscpy ( wszTexDumpDir,
             SK_EvalEnvironmentVars (wszTexDumpDir_RAW).c_str () );

  //
  // Walk custom textures so we don't have to query the filesystem on every
  //   texture load to check if a custom one exists.
  //
  if ( GetFileAttributesW (wszTexDumpDir) !=
         INVALID_FILE_ATTRIBUTES )
  {
    WIN32_FIND_DATA fd     = {  };
    unsigned int    files  =  0UL;
    LARGE_INTEGER   liSize = {  };

    LARGE_INTEGER   liCompressed   = {   };
    LARGE_INTEGER   liUncompressed = {   };

    dll_log->LogEx ( true, L"[DX11TexMgr] Enumerating dumped <%s>...    ",
      SK_ConcealUserDir (std::wstring (wszTexDumpDir).data ()) );

    lstrcatW (wszTexDumpDir, LR"(\*)");

    HANDLE hFind =
      FindFirstFileW (wszTexDumpDir, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          // Dumped Metadata has the extension .dds.txt, do not
          //   include these while scanning for textures.
          if (    StrStrIW (fd.cFileName, L".dds")    &&
               (! StrStrIW (fd.cFileName, L".dds.txt") ) )
          {
            uint32_t top_crc32 = 0x00;
            uint32_t checksum  = 0x00;

            bool compressed = false;

            if (StrStrIW (fd.cFileName, L"Uncompressed_"))
            {
              wchar_t *wszFound =
                StrStrIW (fd.cFileName, L"_");

              if (wszFound && StrStrIW (wszFound + 1, L"_"))
              {
                swscanf ( fd.cFileName,
                            L"Uncompressed_%08X_%08X.dds",
                              &top_crc32,
                                &checksum );
              }

              else
              {
                swscanf ( fd.cFileName,
                            L"Uncompressed_%08X.dds",
                              &top_crc32 );
                checksum = 0x00;
              }
            }

            else
            { if (StrStrIW (fd.cFileName, L"_"))
              {
                wchar_t* wszFound =
                  StrStrIW (fd.cFileName, L"_");

                if (wszFound != nullptr && StrStrIW (wszFound + 1, L"_"))
                {
                  swscanf ( fd.cFileName,
                              L"Compressed_%08X_%08X.dds",
                                &top_crc32,
                                  &checksum );
                }

                else
                {
                  swscanf ( fd.cFileName,
                              L"Compressed_%08X.dds",
                                &top_crc32 );
                  checksum = 0x00;
                }

                compressed = true;
              }
            }

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            if (compressed)
              liCompressed.QuadPart   += fsize.QuadPart;
            else
              liUncompressed.QuadPart += fsize.QuadPart;

            if (textures->dumped_textures.count (top_crc32) >= 1)
              SK_LOG0 ( ( L" >> WARNING: Multiple textures have "
                            L"the same top-level LOD hash (%08X) <<",
                              top_crc32 ), L"DX11TexDmp" );

            if (checksum == 0x00)
            {
              textures->dumped_textures.insert (top_crc32);
            }

            else
            {
              textures->dumped_collisions.insert (
                safe_crc32c (top_crc32, (uint8_t *)(&checksum), 4)
              );
            }
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    textures->dumped_texture_bytes = liSize.QuadPart;

    dll_log->LogEx ( false, L" %lu files (%3.1f MiB -- %3.1f:%3.1f MiB Un:Compressed)\n",
                       files, (double)liSize.QuadPart / (1024.0 * 1024.0),
                                (double)liUncompressed.QuadPart / (1024.0 * 1024.0),
                                  (double)liCompressed.QuadPart /  (1024.0 * 1024.0) );
  }

  wchar_t wszTexInjectDir_RAW [ MAX_PATH + 2 ] = { };
  wchar_t wszTexInjectDir     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexInjectDir_RAW, SK_D3D11_res_root->c_str ());
  lstrcatW (wszTexInjectDir_RAW, LR"(\inject\textures)");

  wcscpy ( wszTexInjectDir,
             SK_EvalEnvironmentVars (wszTexInjectDir_RAW).c_str () );

  if ( GetFileAttributesW (wszTexInjectDir) !=
         INVALID_FILE_ATTRIBUTES )
  {
    dll_log->LogEx ( true, L"[DX11TexMgr] Enumerating injectable <%s>...",
      SK_ConcealUserDir (std::wstring (wszTexInjectDir).data ()) );

    unsigned int    files  =   0;
    LARGE_INTEGER   liSize = {   };

    SK_D3D11_RecursiveEnumAndAddTex (wszTexInjectDir, files, liSize);

    textures->injectable_texture_bytes = liSize.QuadPart;

    dll_log->LogEx ( false, L" %lu files (%3.1f MiB)\n",
                       files, (double)liSize.QuadPart / (1024.0 * 1024.0) );
  }

  wchar_t wszTexInjectDir_FFX_RAW [ MAX_PATH + 2 ] = { };
  wchar_t wszTexInjectDir_FFX     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexInjectDir_FFX_RAW, SK_D3D11_res_root->c_str ());
  lstrcatW (wszTexInjectDir_FFX_RAW, LR"(\inject\textures\UnX_Old)");

  wcscpy ( wszTexInjectDir_FFX,
             SK_EvalEnvironmentVars (wszTexInjectDir_FFX_RAW).c_str () );

  if ( GetFileAttributesW (wszTexInjectDir_FFX) !=
         INVALID_FILE_ATTRIBUTES )
  {
    WIN32_FIND_DATA fd     = {   };
    int             files  =   0;
    LARGE_INTEGER   liSize = {   };

    dll_log->LogEx ( true, L"[DX11TexMgr] Enumerating FFX inject... <%s>",
     SK_ConcealUserDir (std::wstring (wszTexInjectDir_FFX).data ()) );

    lstrcatW (wszTexInjectDir_FFX, LR"(\*)");

    HANDLE hFind =
      FindFirstFileW (wszTexInjectDir_FFX, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          if (StrStrIW (fd.cFileName, L".dds"))
          {
            uint32_t ffx_crc32;

            swscanf (fd.cFileName, L"%08X.dds", &ffx_crc32);

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            textures->injectable_ffx.insert           (ffx_crc32);
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    dll_log->LogEx ( false, L" %li files (%3.1f MiB)\n",
                       files, (double)liSize.QuadPart / (1024.0 * 1024.0) );
  }

  for (auto& it : SK_D3D11_EnumeratedMipmapCache.get ())
  {
    SK_D3D11_AddTexHash (it.first.c_str (), it.second,         0);
    SK_D3D11_AddTexHash (it.first.c_str (), it.second, it.second);
    SK_D3D11_AddDumped  (it.second,         0                   );
    SK_D3D11_AddDumped  (it.second,         it.second           );
  }
}

void
WINAPI
SK_D3D11_SetResourceRoot (const wchar_t* root)
{
  // Non-absolute path (e.g. NOT C:\...\...")
  if (wcschr (root, L':') == nullptr)
  {
         wchar_t wszPath [MAX_PATH + 2] = { };
    wcsncpy_s   (wszPath, MAX_PATH,
                  ( SK_IsInjected () ||
                    config.system.central_repository )
                                    ? SK_GetConfigPath () :
                                      SK_GetRootPath   (),  _TRUNCATE);

    PathAppendW               (wszPath, root);
    SK_D3D11_res_root->assign (wszPath);
  }

  else
    SK_D3D11_res_root->assign (root);
}

void
WINAPI
SK_Resource_SetRoot (const wchar_t* root)
{
  // Redirect while a proper cross-API design is considered ;)
  SK_D3D11_SetResourceRoot (root);
}

std::filesystem::path
SK_Resource_GetRoot (void)
{
  wchar_t     wszPath [ MAX_PATH + 2 ] = { };
  wcsncpy_s ( wszPath,  MAX_PATH,
              SK_EvalEnvironmentVars (SK_D3D11_res_root->c_str ()).c_str (),
                       _TRUNCATE );

  return wszPath;
}

void
WINAPI
SK_D3D11_EnableTexDump (bool enable)
{
  SK_D3D11_dump_textures = enable;
}

void
WINAPI
SK_D3D11_EnableTexInject (bool enable)
{
  SK_D3D11_inject_textures = enable;
}

void
WINAPI
SK_D3D11_EnableTexInject_FFX (bool enable)
{
  SK_D3D11_inject_textures     = enable;
  SK_D3D11_inject_textures_ffx = enable;
}

void
WINAPI
SK_D3D11_EnableTexCache (bool enable)
{
  SK_D3D11_cache_textures = enable;
}

void
WINAPI
SKX_D3D11_MarkTextures (bool x, bool y, bool z)
{
  UNREFERENCED_PARAMETER (x);
  UNREFERENCED_PARAMETER (y);
  UNREFERENCED_PARAMETER (z);
}

void
SK_D3D11_RecursiveEnumAndAddTex  ( const std::wstring   directory,
                                         unsigned int  &files,
                                         LARGE_INTEGER &liSize,
                                         wchar_t       *wszPattern )
{
  WIN32_FIND_DATA fd            = {   };
  wchar_t         wszPath        [MAX_PATH + 2] = { };
  wchar_t         wszFullPattern [MAX_PATH + 2] = { };

  SK_PathCombineW (wszFullPattern, directory.c_str (), L"*");

  HANDLE hFind =
    FindFirstFileW (wszFullPattern, &fd);

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      if (           (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
                                            FILE_ATTRIBUTE_DIRECTORY  &&
          (_wcsnicmp (fd.cFileName, L"."      , MAX_PATH) != 0)       &&
          (_wcsnicmp (fd.cFileName, L".."     , MAX_PATH) != 0)       &&
          (_wcsnicmp (fd.cFileName, L"UnX_Old", MAX_PATH) != 0)   )
      {
        SK_PathCombineW                 (wszPath, directory.c_str (), fd.cFileName);
        SK_D3D11_RecursiveEnumAndAddTex (wszPath, files, liSize, wszPattern);
      }
    } while (FindNextFile (hFind, &fd));

    FindClose (hFind);
  }

  SK_PathCombineW (wszFullPattern, directory.c_str (), wszPattern);

  hFind =
    FindFirstFileW (wszFullPattern, &fd);

  //bool preload =
  //  StrStrIW (directory.c_str (), LR"(\Preload\)") != nullptr;

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
      {
        if (0 == _wcsnicmp (PathFindExtensionW (fd.cFileName), L".dds", MAX_PATH))
        {

        //bool     preloaded = preload;
          uint32_t top_crc32 = 0x00;
          uint32_t checksum  = 0x00;

          const wchar_t* wszFileName = fd.cFileName;

          //if (StrStrIW (wszFileName, L"Preload") == fd.cFileName)
          //{
          //  const size_t skip =
          //    wcslen (L"Preload");
          //
          //  for ( size_t i = 0; i < skip; i++ )
          //    wszFileName = CharNextW (wszFileName);
          //
          //  preloaded = true;
          //}

          if (StrStrIW (wszFileName, L"_"))
          {
            swscanf (wszFileName, L"%08X_%08X.dds", &top_crc32, &checksum);
          }

          else
          {
            swscanf (wszFileName, L"%08X.dds",    &top_crc32);
          }

          ++files;

          LARGE_INTEGER fsize;

          fsize.HighPart = fd.nFileSizeHigh;
          fsize.LowPart  = fd.nFileSizeLow;

          liSize.QuadPart += fsize.QuadPart;

          SK_PathCombineW         (wszPath, directory.c_str (),
                                                wszFileName );
          if (! StrStrIW (         wszPath, L"MipmapCache") )
          {
            SK_D3D11_AddTexHash   (wszPath, top_crc32, 0);

            if (checksum != 0x00)
              SK_D3D11_AddTexHash (wszPath, top_crc32, checksum);

            //if (preloaded)
            //  SK_D3D11_AddTexPreLoad (top_crc32);

          }

          else
          {
            SK_D3D11_MipmapCacheSize += fsize.QuadPart;
            SK_D3D11_EnumeratedMipmapCache->emplace (
              wszPath, top_crc32
            );
          }
        }
      }
    } while (FindNextFileW (hFind, &fd) != 0);
             FindClose     (hFind);
  }
}


std::wstring
SK_D3D11_TexNameFromChecksum ( uint32_t top_crc32,
                               uint32_t checksum,
                               uint32_t ffx_crc32 )
{
  static auto& textures =
    SK_D3D11_Textures;

  if ( textures->injectable_textures.empty () &&
       textures->injectable_ffx.empty      () &&
       textures->tex_hashes_ex.empty       () &&
       textures->tex_hashes.empty          ()    )
  {
    return L"";
  }

  wchar_t wszTex [MAX_PATH + 2] = { };

  static std::wstring res_root (SK_D3D11_res_root.get ());
  static std::wstring eval     (SK_EvalEnvironmentVars (res_root.c_str ()));

  wcsncpy_s ( wszTex,          MAX_PATH,
                eval.c_str (), _TRUNCATE );

  std::wstring hash_name;

  static bool ffx =
    SK_GetModuleHandle (L"unx.dll") != nullptr;

  if (  ( SK_D3D11_inject_textures_ffx ) &&
      (! (hash_name = SK_D3D11_TexHashToName (ffx_crc32, 0x00)).empty ()))
  {
    SK_LOG2 ( ( L"Caching texture with crc32: %x", ffx_crc32 ),
                L" Tex Hash " );

    if (hash_name.find (wszTex) == 0)
      wcsncpy_s ( wszTex,               MAX_PATH,
                    hash_name.c_str (), _TRUNCATE );
    else
      PathAppendW (wszTex, hash_name.c_str ());
  }

  else if (! ( (hash_name = SK_D3D11_TexHashToName (top_crc32, checksum)).empty () &&
               (hash_name = SK_D3D11_TexHashToName (top_crc32, 0x00)    ).empty () ) )
  {
    SK_LOG2 ( ( L"Caching texture with crc32c: %x  (%s) [%s]",
                  top_crc32, hash_name.c_str (), wszTex ),
                L" Tex Hash " );

    if (hash_name.find (wszTex) == 0)
      wcsncpy_s ( wszTex,               MAX_PATH,
                    hash_name.c_str (), _TRUNCATE );
    else
      PathAppendW (wszTex, hash_name.c_str ());
  }

  else if ( SK_D3D11_inject_textures )
  {
    if ( /*config.textures.d3d11.precise_hash &&*/
         SK_D3D11_IsInjectable (top_crc32, checksum) )
    {
      swprintf ( wszTex,
                   LR"(%s\inject\textures\%08X_%08X.dds)",
                     std::wstring (wszTex).c_str (),
                       top_crc32, checksum );
    }

    else if ( SK_D3D11_IsInjectable (top_crc32, 0x00) )
    {
      SK_LOG2 ( ( L"Caching texture with crc32c: %08X", top_crc32 ),
                  L" Tex Hash " );
      swprintf ( wszTex,
                   LR"(%s\inject\textures\%08X.dds)",
                     std::wstring (wszTex).c_str (),
                       top_crc32 );
    }

    else if ( ffx &&
              SK_D3D11_IsInjectable_FFX (ffx_crc32) )
    {
      SK_LOG2 ( ( L"Caching texture with crc32: %08X", ffx_crc32 ),
                  L" Tex Hash " );
      swprintf ( wszTex,
                   LR"(%s\inject\textures\Unx_Old\%08X.dds)",
                     std::wstring (wszTex).c_str (),
                       ffx_crc32 );
    }

    else *wszTex = L'\0';
  }

  // Not a hashed texture, not an injectable texture, skip it...
  else *wszTex = L'\0';

  SK_FixSlashesW (wszTex);

  return wszTex;
}


HRESULT
SK_D3D11_ReloadTexture ( ID3D11Texture2D* pTex,
                         SK_TLS*          pTLS )
{
  static auto& textures =
    SK_D3D11_Textures;

  static auto& rb =
      SK_GetCurrentRenderBackend ();

  HRESULT hr =
    E_UNEXPECTED;

  SK_ScopedBool auto_bool (&pTLS->imgui->drawing);
                            pTLS->imgui->drawing = TRUE;
  {
    SK_D3D11_TexMgr::tex2D_descriptor_s& texDesc2D =
      textures->Textures_2D [pTex];

    std::wstring fname =
      SK_D3D11_TexNameFromChecksum (texDesc2D.crc32c, 0x00);

    if (fname.empty ()) fname = texDesc2D.file_name;

    else
      texDesc2D.file_name = fname;

    if (GetFileAttributes (fname.c_str ()) != INVALID_FILE_ATTRIBUTES)
    {
      DirectX::TexMetadata mdata;
      const LARGE_INTEGER  load_start =
        SK_QueryPerf ();

      if ( SUCCEEDED (
        ( hr = DirectX::GetMetadataFromDDSFile ( fname.c_str (),
      	                                         0x0,    mdata )
        )            )
         )
      {
        DirectX::ScratchImage scratch;

        SK_ComPtr   <ID3D11Resource> pInjTex = nullptr;

        auto pDev =
          rb.getDevice <ID3D11Device> ();

        hr =
          DirectX::LoadFromDDSFile (fname.c_str (), 0x0, &mdata, scratch);

        if (SUCCEEDED (hr))
        {
          SK_ScopedBool decl_tex_scope (
            SK_D3D11_DeclareTexInjectScope (pTLS)
          );

          if ( SUCCEEDED (
               DirectX::CreateTexture ( pDev,
                                              scratch.GetImages     (),
                                              scratch.GetImageCount (),
                                                mdata,
                                             &pInjTex.p )
                         )
             )
          {
            SK_ComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

            if (pDevCtx != nullptr)
            {   pDevCtx->CopyResource (
                   pTex,
                  pInjTex.p
                );
            }

            const LARGE_INTEGER load_end =
              SK_QueryPerf ();

            textures->Textures_2D [pTex].load_time =
                (load_end.QuadPart - load_start.QuadPart);

            return S_OK;
          }
        }
      }
    }
  }

  SK_LOG0 ( ( L" >> Texture Reload Failure (HRESULT: %x)", hr),
              L"DX11TexMgr" );

  return
    E_FAIL;
}


int
SK_D3D11_ReloadAllTextures (void)
{
  static auto& textures =
    SK_D3D11_Textures;

  SK_D3D11_PopulateResourceList (true);

  int count = 0;

  for ( auto& it : textures->Textures_2D )
  {
    if (SK_D3D11_TextureIsCached (it.first))
    {
      if (! (it.second.injected || it.second.discard))
        continue;

      if (SUCCEEDED (SK_D3D11_ReloadTexture (it.first)))
        ++count;
    }
  }

  return count;
}



bool
SK_D3D11_IsStagingCacheable ( D3D11_RESOURCE_DIMENSION  rdim,
                              ID3D11Resource           *pRes,
                              SK_TLS                   *pTLS )
{
  if ( config.textures.d3d11.cache         &&
       config.textures.cache.allow_staging && pRes != nullptr
        &&      D3D11_RESOURCE_DIMENSION_TEXTURE2D == rdim )
  {
    SK_ComQIPtr <ID3D11Texture2D> pTex (pRes);

    if (pTex != nullptr)
    {
      if (SK_D3D11_IsTextureUncacheable (pTex))
        return false;

      D3D11_TEXTURE2D_DESC tex_desc = { };
           pTex->GetDesc (&tex_desc);

      const SK_D3D11_TEXTURE2D_DESC desc (tex_desc);

      if ( (desc.Usage         == D3D11_USAGE_STAGING  ) &&
           (desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) ==
                                  D3D11_CPU_ACCESS_READ )
      {
        if (pTLS == nullptr)
            pTLS  = SK_TLS_Bottom ();

        if ( (! ( pTLS->imgui->drawing == TRUE ||
                  SK_D3D11_IsTexInjectThread (pTLS) ) ) )
        {
          return true;
        }
      }
    }
  }

  return false;
}

//
// Optimization for pushing last access time to the synchronized
//   descriptor list with much lower overhead, uses double-lock-checking
//     and if a texture is accessed multiple times per-frame, only the
//       first usage incurs the expense of an update.
//
void
SK_D3D11_TexMgr::lod_hash_table_s::touch (ID3D11Texture2D *pTex)
{
  static volatile
    ID3D11Texture2D *last_tex   = nullptr;
  static volatile
    ULONG64          last_cache = 0;
    ULONG64          this_frame = SK_GetFramesDrawn ();

  // Fast-path for situations where references are rapidly accumulating for
  //   the same texture.
  if ( ( (void *)pTex       ==
           ReadPointerAcquire ((void **)&last_tex) )
            && ( this_frame ==
             ReadULong64Acquire        (&last_cache) ) )
  {
    return;
  }

  InterlockedExchangePointer ((void **)&last_tex,(void *)pTex);
  WriteULong64Release        (         &last_cache, this_frame);

  auto *pLastFrame =
        &last_frame [pTex];

  if (*pLastFrame != this_frame)
  {   *pLastFrame  = this_frame;

    auto& tex_desc (
      SK_D3D11_Textures->Textures_2D [pTex]
    );

    tex_desc.last_frame = this_frame;
    tex_desc.last_used  = SK_QueryPerf ().QuadPart;
  }
}