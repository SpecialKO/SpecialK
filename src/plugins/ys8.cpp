
// Copyright 2018 Andon "Kaldaien" Coleman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <SpecialK/stdafx.h>

#include <SpecialK/render/d3d11/d3d11_core.h>



#define YS8_VERSION_NUM L"0.8.0"
#define YS8_VERSION_STR L"Ys8 Fixin' Stuff v " YS8_VERSION_NUM

volatile LONG __YS8_init = FALSE;

//extern void
//__stdcall
//SK_SetPluginName (std::wstring name);

using SK_PlugIn_ControlPanelWidget_pfn = void (__stdcall *)(void);

static D3D11Dev_CreateShaderResourceView_pfn _D3D11Dev_CreateShaderResourceView_Original = nullptr;
static D3D11Dev_CreateSamplerState_pfn       _D3D11Dev_CreateSamplerState_Original       = nullptr;
static D3D11Dev_CreateTexture2D_pfn          _D3D11Dev_CreateTexture2D_Original          = nullptr;
static D3D11_RSSetViewports_pfn              _D3D11_RSSetViewports_Original              = nullptr;
static D3D11_Map_pfn                         _D3D11_Map_Original                         = nullptr;
static D3D11_CopyResource_pfn                _D3D11_CopyResource_Original                = nullptr;

static SK_PlugIn_ControlPanelWidget_pfn      SK_PlugIn_ControlPanelWidget_Original       = nullptr;


volatile LONG64 llBytesCopied   = 0;
volatile LONG64 llBytesSkipped  = 0;
volatile LONG   iCopyOpsRun     = 0;
volatile LONG   iCopyOpsSkipped = 0;


struct ys8_cfg_s
{
  struct shadows_s
  {
    sk::ParameterFloat* scale = nullptr;
  } shadows;

  struct mipmaps_s
  {
    sk::ParameterBool* generate    = nullptr;
    sk::ParameterBool* cache       = nullptr;
    sk::ParameterInt*  stream_rate = nullptr;
    sk::ParameterInt*  anisotropy  = nullptr;
  } mipmaps;

  struct cache_s
  {
    uint64_t max_size = UINT64_MAX;
  //FILETIME max_age;
  } cache;

  struct style_s
  {
    sk::ParameterStringW* pixel_style = nullptr;
  } style;

  struct postproc_s
  {
    sk::ParameterBool*  override_postproc = nullptr;
    sk::ParameterFloat* contrast          = nullptr;
    sk::ParameterFloat* vignette          = nullptr;
    sk::ParameterFloat* sharpness         = nullptr;
  } postproc;

  struct performance_s
  {
    sk::ParameterBool* manage_clean_memory    = nullptr;
    sk::ParameterBool* aggressive_memory_mgmt = nullptr;
  } performance;
};

SK_LazyGlobal <ys8_cfg_s> ys8_config;

extern SK_LazyGlobal <
  concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s>
> __SK_D3D11_PixelShader_CBuffer_Overrides;

d3d11_shader_tracking_s::cbuffer_override_s* SK_YS8_CB_Override;



// Level 2 = Loose Files
// Level 1 = Packed Archive
// Level 0 = Coalesced Metadata

struct sk_disk_cache_metadata_s {
  struct
  {
  } size;

  struct
  {
    ULONG decompression;
    ULONG mipmap_creation;
    ULONG read_time;
  } timing;

  struct
  {
    ULONG total_refs;
    ULONG last_time;
    ULONG first_time;
  } usage;
};

SK_LazyGlobal <
  concurrency::concurrent_unordered_map <
    ID3D11Resource *, BOOL
  >
> ys8_dirty_resources;

int ys8_dirty_buckets  = 8;

static bool ManageCleanMemory    = true;
static bool CatchAllMemoryFaults = false;


static int   max_anisotropy            = 4;
static bool  anisotropic_filter        = true;

static bool  disable_negative_lod_bias = true;
static float explicit_lod_bias         = 0.0f;
static bool  nearest_neighbor          = false;



int SK_D3D11_TexStreamPriority = 1; // 0=Low  (Pop-In),
                                    // 1=Balanced,
                                    // 2=High (No Pop-In)



extern std::wstring
DescribeResource (ID3D11Resource* pRes);

//extern
//HRESULT
//WINAPI
//D3D11Dev_CreateSamplerState_Override (
//  _In_            ID3D11Device        *This,
//  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
//  _Out_opt_       ID3D11SamplerState **ppSamplerState );
//
//extern
//HRESULT
//WINAPI
//D3D11Dev_CreateShaderResourceView_Override (
//  _In_           ID3D11Device                     *This,
//  _In_           ID3D11Resource                   *pResource,
//  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
//  _Out_opt_      ID3D11ShaderResourceView        **ppSRView );
//
//extern
//HRESULT
//WINAPI
//D3D11Dev_CreateTexture2D_Override (
//  _In_            ID3D11Device           *This,
//  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
//  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
//  _Out_opt_       ID3D11Texture2D        **ppTexture2D );
//
//extern
//HRESULT
//STDMETHODCALLTYPE
//D3D11_Map_Override (
//   _In_ ID3D11DeviceContext      *This,
//   _In_ ID3D11Resource           *pResource,
//   _In_ UINT                      Subresource,
//   _In_ D3D11_MAP                 MapType,
//   _In_ UINT                      MapFlags,
//_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource );
//
//extern
//void
//STDMETHODCALLTYPE
//D3D11_CopyResource_Override (
//       ID3D11DeviceContext *This,
//  _In_ ID3D11Resource      *pDstResource,
//  _In_ ID3D11Resource      *pSrcResource );
//
//extern
//void
//WINAPI
//D3D11_RSSetViewports_Override (
//        ID3D11DeviceContext* This,
//        UINT                 NumViewports,
//  const D3D11_VIEWPORT*      pViewports );

extern
void
__stdcall
SK_PlugIn_ControlPanelWidget (void);


std::set <uint32_t>
SK_Cache_VisitMetadata (void *pAlgo)
{
  DBG_UNREFERENCED_PARAMETER (pAlgo);
  return std::set <uint32_t> { 0ui32 };
}



float __SK_YS8_ShadowScale = 1.0f;

static void
SK_YS8_RecursiveFileExport (
                                          const wchar_t *  wszRoot,
                                          const wchar_t *  wszSubDir,
                             std::vector <const wchar_t *> patterns )
{
 const
  auto
  _TryPatterns =
  [&](const wchar_t* wszFile) ->
  bool
  {
    for (auto pattern : patterns)
      if (StrStrIW (wszFile, pattern)) return true;

    return false;
  };

  wchar_t   wszPath [MAX_PATH + 2] = { };
  swprintf (wszPath, LR"(%s\%s\*)", wszRoot, wszSubDir);

  WIN32_FIND_DATA fd          = {   };
  HANDLE          hFind       =
    FindFirstFileW ( wszPath, &fd);

  if (hFind == INVALID_HANDLE_VALUE) { return; }

  do
  {
    if ( wcscmp (fd.cFileName, L".")  == 0 ||
         wcscmp (fd.cFileName, L"..") == 0 )
    {
      continue;
    }

    if ( (  (            fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
                                               FILE_ATTRIBUTE_DIRECTORY) &&
           _TryPatterns (fd.cFileName)
       )
    {
      std::wstring in_file =
        SK_FormatStringW ( LR"(%s\%s\%s)",
                            wszRoot, wszSubDir,
                              fd.cFileName );

      FILE*    fIN  =
        _wfopen   (in_file.c_str (), L"rb+");

      if (fIN)
      {
        const size_t size = gsl::narrow_cast <size_t> (
          SK_File_GetSize (in_file.c_str ())
        );

        uint8_t *data =
          new uint8_t [size] { };

        fread  (data, size, 1, fIN);
        fclose (               fIN);

        for (size_t i = 0; i < size; i++)
        {
          data [i] = (  (data [i] << 4U) |
                       ~(data [i] >> 4U) & 0xFU )
                                         & 0xFF;
        }

        std::wstring out_file =
          SK_FormatStringW ( LR"(%s\SK_Export\%s\%s)",
                              wszRoot, wszSubDir,
                                fd.cFileName );

        SK_CreateDirectories (out_file.c_str ());

        FILE* fOUT =
          _wfopen ( out_file.c_str (), L"wb+" );

        if (fOUT)
        {
          fwrite (data, size, 1, fOUT);
          fclose (               fOUT);
        }

        delete [] data;
      }
    }

    else if ( _wcsicmp (fd.cFileName, L"SK_Export") != 0 )
    {
      wchar_t   wszDescend [MAX_PATH + 2] = { };
      swprintf (wszDescend, LR"(%s\%s)", wszSubDir, fd.cFileName);

      SK_YS8_RecursiveFileExport (wszRoot, wszDescend, patterns);
    }
  } while (FindNextFile (hFind, &fd));

  FindClose (hFind);
}

static std::pair <INT, uint64_t>
SK_YS8_RecursiveFileImport (
                                          const wchar_t *  wszRoot,
                                          const wchar_t *  wszSubDir,
                             std::vector <const wchar_t *> patterns,
                                                bool       backup )
{
  std::pair <INT, uint64_t> work_done { 0, 0ULL };

const
 auto
  _TryPatterns =
  [&](const wchar_t* wszFile) ->
  bool
  {
    for (auto pattern : patterns)
      if (StrStrIW (wszFile, pattern)) return true;

    return false;
  };

  wchar_t   wszPath [MAX_PATH + 2] = { };
  swprintf (wszPath, LR"(%s\%s\*)", wszRoot, wszSubDir);

  WIN32_FIND_DATA fd          = {   };
  HANDLE          hFind       =
    FindFirstFileW ( wszPath, &fd);

  if (hFind == INVALID_HANDLE_VALUE) { return { 0, 0ULL }; }

  do
  {
    if ( wcscmp (fd.cFileName, L".")  == 0 ||
         wcscmp (fd.cFileName, L"..") == 0 )
    {
      continue;
    }

    if ( (  (            fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
                                               FILE_ATTRIBUTE_DIRECTORY) &&
           _TryPatterns (fd.cFileName)
       )
    {
      std::wstring in_file =
        SK_FormatStringW ( LR"(%s\SK_Import\%s\%s)",
                            wszRoot, wszSubDir,
                              fd.cFileName );

      const size_t size = gsl::narrow_cast <size_t> (
        SK_File_GetSize (in_file.c_str ())
      );

      if (size > 0)
      {
        FILE*    fIN  =
          _wfopen         (in_file.c_str (), L"rb+");

        uint8_t *data =
          new uint8_t [size] { };

        if (data != nullptr)
        {
          fread  (data, size, 1, fIN);
          fclose (               fIN);

          for (size_t i = 0; i < size; i++)
          {
            data [i] = ( ~(data [i] << 4U) & 0xF0U |
                          (data [i] >> 4U) &  0xFU ) & 0xFFU;
          }

          std::wstring out_file =
            SK_FormatStringW ( LR"(%s\%s\%s)",
                                wszRoot, wszSubDir,
                                  fd.cFileName );

          if (backup)
          {
            std::wstring backup_file =
              SK_FormatStringW ( LR"(%s\SK_Backup\%s\%s)",
                                  wszRoot, wszSubDir,
                                    fd.cFileName );

            SK_CreateDirectories ( backup_file.c_str () );
            SK_File_MoveNoFail   ( out_file.c_str    (),
                                   backup_file.c_str () );
          }

          FILE* fOUT =
            _wfopen ( out_file.c_str (), L"wb+" );

          fwrite (data, size, 1, fOUT);
          fclose (               fOUT);

          delete [] data;
        }

        work_done.first  += 1;
        work_done.second += size;
      }
    }

    else if ( _wcsicmp (fd.cFileName, L"SK_Export") != 0 )
    {
      wchar_t   wszDescend [MAX_PATH + 2] = { };
      swprintf (wszDescend, LR"(%s\%s)", wszSubDir, fd.cFileName);

      const auto recursive_work =
        SK_YS8_RecursiveFileImport (wszRoot, wszDescend, patterns, backup);

      work_done.first  += recursive_work.first;
      work_done.second += recursive_work.second;
    }
  } while (FindNextFile (hFind, &fd));

  FindClose (hFind);

  return work_done;
}


static SK_LazyGlobal <std::unordered_map <std::wstring, uint64_t>> _SK_YS8_CachedDirSizes;

static
uint64_t
_SK_RecursiveFileSizeProbe (const wchar_t *wszDir, bool top_lvl = false)
{
  if ( top_lvl || _SK_YS8_CachedDirSizes->count (wszDir) > 0 )
  {
    uint64_t cached_size = _SK_YS8_CachedDirSizes.get()[wszDir];

    if (cached_size > 0ULL)
      return cached_size;
  }

  uint64_t size = 0;

  wchar_t   wszPath [MAX_PATH + 2] = { };
  swprintf (wszPath, LR"(%s\*)", wszDir);

  WIN32_FIND_DATA fd          = {   };
  HANDLE          hFind       =
    FindFirstFileW ( wszPath, &fd );

  if (hFind == INVALID_HANDLE_VALUE) { return 0ULL; }

  do
  {
    if ( wcscmp (fd.cFileName, L".")  == 0 ||
         wcscmp (fd.cFileName, L"..") == 0 )
    {
      continue;
    }

    if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
                               FILE_ATTRIBUTE_DIRECTORY)
    {
      size +=
        ULARGE_INTEGER { fd.nFileSizeLow,
                         fd.nFileSizeHigh }.QuadPart;
    }

    else
    {
      wchar_t   wszDescend [MAX_PATH + 2] = { };
      swprintf (wszDescend, LR"(%s\%s)", wszDir, fd.cFileName);

      size += _SK_RecursiveFileSizeProbe (wszDescend);
    }
  } while (FindNextFile (hFind, &fd));

  FindClose (hFind);

  if (top_lvl && size > 0ULL)
    _SK_YS8_CachedDirSizes.get ()[wszDir] = size;

  return size;
};

bool b_66b35959 = false;
bool b_9d665ae2 = false;
bool b_b21c8ab9 = false;
bool b_6bb0972d = false;
bool b_05da09bd = true;

extern LONG SK_D3D11_Resampler_GetActiveJobCount  (void);
extern LONG SK_D3D11_Resampler_GetWaitingJobCount (void);
extern LONG SK_D3D11_Resampler_GetRetiredCount    (void);
extern LONG SK_D3D11_Resampler_GetErrorCount      (void);

void
__stdcall
SK_YS8_ControlPanel (void)
{
  if (ImGui::CollapsingHeader ("Ys VIII Lacrimosa of Dana", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    bool changed = false;

    const bool tex_manage =
      ImGui::CollapsingHeader ("Texture Management##Ys8", ImGuiTreeNodeFlags_DefaultOpen);

    const LONG jobs =
      SK_D3D11_Resampler_GetActiveJobCount () + SK_D3D11_Resampler_GetWaitingJobCount ();

    static DWORD dwLastActive = 0;

    if (jobs != 0 || dwLastActive > SK_timeGetTime () - 500)
    {
      if (jobs > 0)
        dwLastActive = SK_timeGetTime ();

      if (jobs > 0)
      {
        ImGui::PushStyleColor ( ImGuiCol_Text,
                                  ImColor::HSV ( 0.4f - ( 0.4f * (
                                                 SK_D3D11_Resampler_GetActiveJobCount ()
                                                                 ) /
                                               static_cast <float> (jobs)
                                                        ), 0.15f,
                                                             1.0f
                                               ).Value
                              );
      }
      else
      {
        ImGui::PushStyleColor ( ImGuiCol_Text,
                                  ImColor::HSV ( 0.4f - ( 0.4f * (SK_timeGetTime () - dwLastActive) /
                                                          500.0f ),
                                                   1.0f,
                                                     0.8f
                                               ).Value
                              );
      }

      ImGui::SameLine       ();
      if (SK_D3D11_Resampler_GetErrorCount ())
        ImGui::Text         ("       Textures ReSampling (%li / %li) { Error Count: %li }",
                               SK_D3D11_Resampler_GetActiveJobCount (),
                                 jobs,
                               SK_D3D11_Resampler_GetErrorCount     ()
                            );
      else
        ImGui::Text         ("       Textures ReSampling (%li / %li)",
                               SK_D3D11_Resampler_GetActiveJobCount (),
                                 jobs );
      ImGui::PopStyleColor  ();
    }

    if (tex_manage)
    {
      ImGui::TreePush    ("");
      ImGui::BeginGroup  ();
      changed |= ImGui::Checkbox    ("Generate Mipmaps", &config.textures.d3d11.generate_mips);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    ();
        ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.5f, 0.f, 1.f, 1.f));
        ImGui::TextUnformatted ("Builds Complete Mipchains (Mipmap LODs) for all Textures");
        ImGui::Separator       ();
        ImGui::PopStyleColor   ();
        ImGui::Bullet          (); ImGui::SameLine ();
        ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.15f, 1.0f, 1.0f));
        ImGui::TextUnformatted ("SIGNIFICANTLY");
        ImGui::PopStyleColor   (); ImGui::SameLine ();
        ImGui::TextUnformatted ("reduces texture aliasing");
        ImGui::BulletText      ("May increase load-times and/or hurt compatibility with third-party software");
        ImGui::EndTooltip      ();
      }

      if (config.textures.d3d11.generate_mips)
      {
        ImGui::SameLine ();

        if (
          ImGui::Combo ("Texture Streaming Priority", &SK_D3D11_TexStreamPriority, "Short load screens; (Pop-In)\0"
                                                                                   "Balanced\0"
                                                                                   "Long load screens; (Anti-Pop-In)\0\0")
           )
        {
          ys8_config->mipmaps.stream_rate->store (SK_D3D11_TexStreamPriority);
          changed = true;
        }

        extern uint64_t SK_D3D11_MipmapCacheSize;

        if (ImGui::Checkbox ("Cache Mipmaps to Disk", &config.textures.d3d11.cache_gen_mips))
        {
          changed = true;

          ys8_config->mipmaps.cache->store (config.textures.d3d11.cache_gen_mips);
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Eliminates almost all Texture Pop-In");
          ImGui::Separator    ();
          ImGui::BulletText   ("Will quickly consume a LOT of disk space!");
          ImGui::EndTooltip   ();
        }

        static wchar_t wszPath [ MAX_PATH + 2 ] = { };

        if (*wszPath == L'\0')
        {
          wcscpy ( wszPath,
                     SK_EvalEnvironmentVars (SK_D3D11_res_root->c_str ()).c_str () );

          lstrcatW (wszPath, LR"(\inject\textures\MipmapCache\)");
          lstrcatW (wszPath, SK_GetHostApp ());
          lstrcatW (wszPath, LR"(\)");
        }

        if (SK_D3D11_MipmapCacheSize > 0)
        {
              ImGui::SameLine (               );
          if (ImGui::Button   (" Purge Cache "))
          {
            SK_D3D11_MipmapCacheSize -= SK_DeleteTemporaryFiles (wszPath, L"*.dds");

            assert ((int64_t)SK_D3D11_MipmapCacheSize > 0LL);

            void
            WINAPI
            SK_D3D11_PopulateResourceList (bool refresh);

            SK_D3D11_PopulateResourceList (true);
          }

          if (ImGui::IsItemHovered ())
              ImGui::SetTooltip    ("%ws", wszPath);
        }

        // For safety, never allow a user to touch the final 256 MiB of storage on their device
        const ULONG FILESYSEM_RESERVE_MIB = 256UL;

        if (SK_D3D11_MipmapCacheSize > 0)
        {
          ImGui::SameLine ();
          ImGui::Text     ("Current Cache Size: %.2f MiB", (double)SK_D3D11_MipmapCacheSize / (1024.0 * 1024.0));
        }

        static uint64_t       last_MipmapCacheSize =         0ULL;
        static ULARGE_INTEGER ulBytesAvailable     = { 0UL, 0UL },
                              ulBytesTotal         = { 0UL, 0UL };

        //
        // GetDiskFreeSpaceEx has highly unpredictable call overhead
        //
        //   ... so try to be careful with it!
        //
        if (SK_D3D11_MipmapCacheSize != last_MipmapCacheSize)
        {
            last_MipmapCacheSize      = SK_D3D11_MipmapCacheSize;

          GetDiskFreeSpaceEx ( wszPath, &ulBytesAvailable,
                                        &ulBytesTotal, nullptr);
        }

        //static int min_reserve = 1024;
        //       int upper_lim = std::min ( (int)(ulBytesTotal.QuadPart / (1024ULL * 1024ULL)),
        //                       std::max (
        //            min_reserve,
        //      (int)(ulBytesAvailable.QuadPart / (1024ULL * 1024ULL)))
        //);
        //
        //if (config.textures.d3d11.cache_gen_mips)
        //{
        //  ImGui::SliderInt ( "Minimum Storage Reserve###SK_YS8_MipCacheSize",
        //                       &min_reserve, FILESYSEM_RESERVE_MIB, upper_lim,
        //                                                          SK_WideCharToUTF8 (
        //                                                            SK_File_SizeToStringF (min_reserve * 1024ULL * 1024ULL, 2, 3)
        //                                                          ).c_str () );
        //
        //  if (ImGui::IsItemHovered ())
        //      ImGui::SetTooltip ("Caching will immediately cease if your available storage capacity drops below this safetynet.");
        //}

        if (SK_D3D11_MipmapCacheSize > 0)
        {
          ImGui::ProgressBar ( static_cast <float> (static_cast <long double> (ulBytesAvailable.QuadPart) /
                                                    static_cast <long double> (ulBytesTotal.QuadPart)       ),
                                 ImVec2 (-1.0f, 0.0f),
              SK_WideCharToUTF8 (
                std::wstring (SK_File_SizeToStringF (ulBytesAvailable.QuadPart, 2, 3).data ()) + L" Remaining Storage Capacity"
              ).c_str ()
          );
        }
      }
      ImGui::EndGroup  ();
      ImGui::Separator ();

      ImGui::Checkbox ("Anisotropic Filtering", &anisotropic_filter);

      if (anisotropic_filter)
      {
        ImGui::SameLine ();

        changed |= ImGui::SliderInt ("Maximum Anisotropy", &max_anisotropy, 1, 16);

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ("Pro Tip:  Modern GPUs can do 4x Anisotropic Filtering without any performance penalties.");
        }

        if (changed) ys8_config->mipmaps.anisotropy->store (max_anisotropy);
      }

      //if (! SK_IsInjected ())
      {
        enum {
          Retro = 0,
          Clean = 1,
          Sharp = 2
        };

        static int current_style = nearest_neighbor ?
                                   0                : explicit_lod_bias < -1.0f ? 2 : 1;

        if ( ImGui::Combo ( "Art Style", &current_style, "Retro Pixel Art\0"
                                                         "Anti-Aliased\0"
                                                         "Sharpen (Tex. Shimmer)\0\0" ) )
        {
          changed = true;

          std::wstring style;

          switch (current_style)
          {
            case Retro:
              max_anisotropy            =    8;
              anisotropic_filter        = true;

              disable_negative_lod_bias =    FALSE;
              explicit_lod_bias         = 3.16285f;
              nearest_neighbor          =     TRUE;
              style                     = L"Retro";

              b_66b35959 = true;
              b_9d665ae2 = false;
              b_b21c8ab9 = false;
              b_6bb0972d = true;
              b_05da09bd = false;

              break;

            case Clean:
              max_anisotropy            =   16;
              anisotropic_filter        = true;

              disable_negative_lod_bias =     TRUE;
              explicit_lod_bias         =  -0.001f;
              nearest_neighbor          =    FALSE;
              style                     = L"Clean";

              b_66b35959 = false;
              b_9d665ae2 = false;
              b_b21c8ab9 = false;
              b_6bb0972d = false;
              b_05da09bd = false;

              break;

            case Sharp:
              max_anisotropy            =   14;
              anisotropic_filter        = true;

              disable_negative_lod_bias =    FALSE;
              explicit_lod_bias         = -2.9678f;
              nearest_neighbor          =    FALSE;
              style                     = L"Sharp";

              b_66b35959 = false;
              b_9d665ae2 = false;
              b_b21c8ab9 = false;
              b_6bb0972d = false;
              b_05da09bd = true;
              break;
          }

          ys8_config->style.pixel_style->store (style);
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("It may be necessary to load a save file before these completely take affect.");

        ImGui::SameLine (); ImGui::Checkbox ("Shader 0###YS8_Shader0", &b_66b35959);
        ImGui::SameLine (); ImGui::Checkbox ("1###YS8_Shader1",        &b_9d665ae2);
        ImGui::SameLine (); ImGui::Checkbox ("2###YS8_Shader2",        &b_b21c8ab9);
        ImGui::SameLine (); ImGui::Checkbox ("3###YS8_Shader3",        &b_6bb0972d);
        ImGui::SameLine (); ImGui::Checkbox ("4###YS8_Shader4",        &b_05da09bd);
      }

      if (config.system.log_level > 0)
      {
        ImGui::Checkbox      ("Disable Negative LOD Bias", &disable_negative_lod_bias);

        ImGui::SameLine      ();
        ImGui::BeginGroup    ();

        if (ImGui::TreeNode  ("Debug Settings###SK_YS8_DEBUG"))
        {
          ImGui::Checkbox    ("Point-Sampling",    &nearest_neighbor);
          ImGui::SliderFloat ("Explicit LOD Bias", &explicit_lod_bias, -3.f, 3.f);
          ImGui::TreePop     ();
        }
        ImGui::EndGroup      ();
      }

      ImGui::TreePop         ();
    }

    if (ImGui::CollapsingHeader ("Performance###SK_YS8_PERF", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      if (ManageCleanMemory)
      {
        const LONG64 llTotalBytes =
          ReadAcquire64 (&llBytesSkipped) + ReadAcquire64 (&llBytesCopied);

        LONG64 llSkipped =
          ReadAcquire64 (&llBytesSkipped);

        ImGui::PushStyleColor (ImGuiCol_PlotHistogram, (ImVec4&&)ImColor::HSV ((float)std::min ((long double)1.0, (long double)llSkipped / (long double)(llTotalBytes)) * 0.278f, 0.88f, 0.333f));

        ImGui::ProgressBar ( float (((long double)llSkipped) /
                                     (long double)(llTotalBytes)), ImVec2 (-1.0f, 0.0f),
          SK_FormatString ("%ws out of %ws were avoided\t\t\t\tDirty Hash (%ws :: Load=%4.2f)", std::wstring (SK_File_SizeToString (llSkipped).                                              data ()).c_str (),
                                                                                                std::wstring (SK_File_SizeToString (llTotalBytes).                                           data ()).c_str (),
                                                                                                std::wstring (SK_File_SizeToString (ys8_dirty_resources->size () * (sizeof (uintptr_t) + 1)).data ()).c_str (),
                                                                                                                      ys8_dirty_resources->load_factor ()).c_str ());
        ImGui::PopStyleColor ();
      }


      if (ImGui::Checkbox ("Prevent the engine from sending copies of text that has not changed to the GPU.", &ManageCleanMemory))
      {
        changed = true;

        ys8_config->performance.manage_clean_memory->store (ManageCleanMemory);
      }

      if (ManageCleanMemory)
      {
        if (ImGui::Checkbox ("Aggressive Memory Optimization", &CatchAllMemoryFaults))
        {
          changed = true;

          ys8_config->performance.aggressive_memory_mgmt->store (CatchAllMemoryFaults);
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ("May provide an FPS boost in combat with post-processing, but may also cause visual artifacts -- EXPERIMENTAL!");
        }
      }

      ImGui::TreePop (   );
    }

    if (ImGui::CollapsingHeader ("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      static
        std::unordered_map <int, float> fwd_sel {
          { 0l, 1.f }, { 1l, 2.f },
          { 2l, 3.f }, { 3l, 4.f }
        };

      static
        std::unordered_map <float, int> rev_sel {
          { 1.f, 0l }, { 2.f, 1l },
          { 3.f, 2l }, { 4.f, 3l }
        };

      static int sel =
             rev_sel.count ( __SK_YS8_ShadowScale ) ?
             rev_sel [       __SK_YS8_ShadowScale]  : 0;

      static int orig_sel = sel;

      if ( ImGui::Combo ( "Shadow Resolution###SK_YS8_ShadowRes", &sel,
                            "Normal\0"
                            "High\0"
                            "Very High\0"
                            "Ultra\0\0" ) )
      {
        changed = true;

        ys8_config->shadows.scale->store (
          (__SK_YS8_ShadowScale = fwd_sel [sel])
        );
      }

      if (orig_sel != sel)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Game Restart Required");
        ImGui::PopStyleColor  ();
      }

      if (__SK_YS8_ShadowScale > 3)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.14f, .8f, .9f));
        ImGui::BulletText     ("Strongly consider a lower quality setting for best performance");
        ImGui::PopStyleColor  ();
      }

      ImGui::TreePop  ();
    }

    if (changed)
    {
      iSK_INI* pINI =
        SK_GetDLLConfig ();

      pINI->write (pINI->get_filename ());
    }


    const bool manage_files =
      ImGui::CollapsingHeader ("File Management");

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("Import and Export encrypted game assets -- (not all data is \"encrypted\")");
      ImGui::Separator    ();
      ImGui::BulletText   ("Creates a new directory structure, \"SK_Export\" with the decrypted files");
      ImGui::BulletText   ("Modify exported files and then use the \"SK_Import\" directory to import your modifications");
      ImGui::Separator    ();
      ImGui::TreePush     ("");ImGui::TreePush ("");
      ImGui::TextColored  ( ImColor::HSV (0.2F, 0.96f, 0.98F),
                            "These operations may take a long time to complete and rendering will halt during Import/Export." );
      ImGui::TreePop      ();ImGui::TreePop    ();
      ImGui::EndTooltip   ();
    }

    static DWORD   dwLen                     =   0;
    static wchar_t wszWorkDir [MAX_PATH + 2] = { };

    if (dwLen == 0)
    {
      dwLen = MAX_PATH;
      dwLen = GetCurrentDirectoryW (dwLen, wszWorkDir);
    }


const
 auto
  _LinkToExportedDataFolder =
   [&](const wchar_t *wszDir) -> bool
    {
      std::string eval_dir_utf8 (
        SK_FormatString ( R"(%ws\SK_Export\%ws##SK_YS8_ExportDataDir)",
                             wszWorkDir,   wszDir )
      );

      std::wstring probe_dir =
        SK_FormatStringW ( LR"(%ws\SK_Export\%ws)",
                             wszWorkDir, wszDir );

      bool activate_menu =
        ImGui::MenuItem (eval_dir_utf8.c_str (), SK_WideCharToUTF8 (
                                                   SK_File_SizeToStringF (
                                                     _SK_RecursiveFileSizeProbe ( probe_dir.c_str (), true ), 3, 2
                                                   ).data ()
                                                 ).c_str ()
                        );

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Right-click to reclaim this disk space");

      if ( SK_ImGui_IsItemRightClicked () && _SK_YS8_CachedDirSizes.get ()[probe_dir] != 0 )
      {
        _SK_YS8_CachedDirSizes.get ()[probe_dir] =
          static_cast <uint64_t> (
            std::max ( 0LL,
              (int64_t)( _SK_YS8_CachedDirSizes.get ()[probe_dir] -
                             SK_DeleteTemporaryFiles ( probe_dir.c_str (), L"*.*" ) )
            )
          );

        return false;
      }

      if (activate_menu)
      {
        SK_ShellExecuteW ( nullptr,
          L"explore",
            probe_dir.c_str (),
              nullptr, nullptr,
                    SW_NORMAL
        );
      }

      return true;
    };

    static std::map <std::wstring, int> has_exports;

const
  auto
    _SK_YS8_ImportExportUI =
    [&] ( const wchar_t* export_dir, const char* szExportButton,
                                     const char* szImportButton,
          const wchar_t* wszSubdir,
          std::vector <const wchar_t*> extensions )->
    void
    {
      static bool backup = true;

      ImGui::TreePush ("");

      if (has_exports.count (export_dir) == 0)
      {
        has_exports [export_dir] =
          ( GetFileAttributesW (export_dir) != INVALID_FILE_ATTRIBUTES );
      }

      if (ImGui::Button (szExportButton))
      {
        SK_YS8_RecursiveFileExport (wszWorkDir, wszSubdir, extensions);

        has_exports [export_dir] = 0;
      }

      ImGui::SameLine ();

      if (ImGui::Button (szImportButton))
      {
        const auto work =
          SK_YS8_RecursiveFileImport (wszWorkDir, wszSubdir, extensions, backup);

        SK_ImGui_Warning (
          SK_FormatStringW ( L"Imported %lu files and %s of data into %s\\%s ; [%s]",
                               work.first,
         SK_File_SizeToString (work.second).data (),
                               wszWorkDir, wszSubdir, backup ? L"With Backups" :
                                                                     L"No Backup Made!" ).c_str ()
        );
      } ImGui::SameLine (); ImGui::Checkbox ("Backup Imported Files", &backup);

      if (has_exports [export_dir])
          has_exports [export_dir] = _LinkToExportedDataFolder (wszSubdir);

      ImGui::TreePop  (  );
    };


    if (manage_files)
    {
      static bool backup = true;

      ImGui::TreePush       ("");
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

      if (ImGui::CollapsingHeader ("Audio Assets"))
      {
        ImGui::TreePush       ("");
        ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
        ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
        ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

        if (ImGui::CollapsingHeader ("Voices (English)"))
        {
          _SK_YS8_ImportExportUI ( SK_FormatStringW (LR"(%s\SK_Export\voice\en)", wszWorkDir).c_str (),
                                     "Export##SK_YS8_EXP_VO_EN",
                                     "Import##SK_YS8_IMP_VO_EN",  LR"(voice\en)", { L".wav" } );
        }

        if (ImGui::CollapsingHeader ("Voices (Japanese)"))
        {
          _SK_YS8_ImportExportUI ( SK_FormatStringW (LR"(%s\SK_Export\voice\ja)", wszWorkDir).c_str (),
                                     "Export##SK_YS8_EXP_VO_JA",
                                     "Import##SK_YS8_IMP_VO_JA",  LR"(voice\ja)", { L".wav" } );
        }


        if (ImGui::CollapsingHeader ("Sound Effects"))
        {
          _SK_YS8_ImportExportUI ( SK_FormatStringW (LR"(%s\SK_Export\se)", wszWorkDir).c_str (),
                                     "Export##SK_YS8_EXP_SE",
                                     "Import##SK_YS8_IMP_SE",  LR"(se)", { L".wav" } );
        }

        if (ImGui::CollapsingHeader ("Music"))
        {
          _SK_YS8_ImportExportUI ( SK_FormatStringW (LR"(%s\SK_Export\bgm)", wszWorkDir).c_str (),
                                     "Export##SK_YS8_EXP_BGM",
                                     "Import##SK_YS8_IMP_BGM",  LR"(bgm)", { L".ogg" } );
        }

        ImGui::PopStyleColor (3);
        ImGui::TreePop       ( );
      }


      if (ImGui::CollapsingHeader ("In-Game Text"))
      {
        _SK_YS8_ImportExportUI ( SK_FormatStringW (LR"(%s\SK_Export\text)", wszWorkDir).c_str (),
                                   "Export##SK_YS8_EXP_TEXT",
                                   "Import##SK_YS8_IMP_TEXT",  LR"(text)",
                                      { L".csv", L".tbb", L".sai",
                                        L".sab", L".tbl" } );
      }


      ///if (ImGui::CollapsingHeader ("Scripted Events"))
      ///{
      ///  ImGui::TreePush ("");
      ///
      ///  ImGui::TreePop  (  );
      ///}


      bool expand_headers = ImGui::CollapsingHeader ("C/C++ Data Structures");

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ( );
        ImGui::Text         ("Some of these headers are actually loaded by the game at runtime.");
        ImGui::Separator    ();
        ImGui::BulletText   ((const char *)
                             u8"If you can read 日本語, the C code is (mostly) Shift-JIS encoded, with a few UTF-8");
        ImGui::BulletText   (  "If ancient Chinese squiggles confuse you, the C headers are still useful for CheatEngine");
        ImGui::EndTooltip   ();
      }

      if (expand_headers)
      {
        _SK_YS8_ImportExportUI ( SK_FormatStringW (LR"(%s\SK_Export\inc)", wszWorkDir).c_str (),
                                   "Export##SK_YS8_EXP_INC",
                                   "Import##SK_YS8_IMP_INC",  LR"(inc)",
                                      { L".h", L".txt" } );
      }

      ImGui::PopStyleColor (3);
      ImGui::TreePop       ( );
    }

    if (ImGui::CollapsingHeader ("Post-Processing###SK_YS8_POSTPROC"))
    {
      ImGui::TreePush ("");

      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

      changed |= ImGui::Checkbox ("Manual Override", &SK_YS8_CB_Override->Enable);

      if (SK_YS8_CB_Override->Enable)
      {
        ImGui::TreePush    ("");
        ImGui::BeginGroup  ();
        changed |= ImGui::SliderFloat ("Contrast",    SK_YS8_CB_Override->Values,     0.0f, 1.0f);
        changed |= ImGui::SliderFloat ("Vignette",   &SK_YS8_CB_Override->Values [1], 0.0f, 1.0f);
        changed |= ImGui::SliderFloat ("Sharpness",  &SK_YS8_CB_Override->Values [3], 0.0f, 3.0f);
        ImGui::EndGroup    ();
        ImGui::TreePop     ();
      }

      if (changed)
      {
        auto& postproc =
          ys8_config->postproc;

        postproc.override_postproc->store (SK_YS8_CB_Override->Enable);
        postproc.contrast->store          (SK_YS8_CB_Override->Values [0]);
        postproc.vignette->store          (SK_YS8_CB_Override->Values [1]);
        postproc.sharpness->store         (SK_YS8_CB_Override->Values [3]);
      }

      ImGui::PopStyleColor (3);
      ImGui::TreePop       ( );
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }
}



constexpr          uint32_t
SK_NextPowerOfTwo (uint32_t n)
{
   n--;    n |= n >> 1;
           n |= n >> 2;   n |= n >> 4;
           n |= n >> 8;   n |= n >> 16;

  return ++n;
}


__declspec (noinline)
void
STDMETHODCALLTYPE
SK_YS8_RSSetViewports (
        ID3D11DeviceContext* This,
        UINT                 NumViewports,
  const D3D11_VIEWPORT*      pViewports )
{
  if (NumViewports > 0)
  {
    // Safer than alloca
    D3D11_VIEWPORT* vps =
   (D3D11_VIEWPORT *)SK_TLS_Bottom ()->scratch_memory->cmd.alloc (
                                         sizeof (D3D11_VIEWPORT) * NumViewports
                     );

    for (UINT i = 0; i < NumViewports; i++)
    {
      vps [i] = pViewports [i];

      if ( vps [i].Width  == 4096.0f &&
           vps [i].Height == 4096.0f )
      {
          vps [i].Width  *= __SK_YS8_ShadowScale;
          vps [i].Height *= __SK_YS8_ShadowScale;
      }
    }

    _D3D11_RSSetViewports_Original (This, NumViewports, vps);
  }

  else
    _D3D11_RSSetViewports_Original (This, NumViewports, pViewports);
}


using CloseHandle_pfn = BOOL   (WINAPI *)(_In_     HANDLE                hObject);
using OpenFile_pfn    = HFILE  (WINAPI *)(_In_     LPCSTR                lpFileName,
                                          _Out_    LPOFSTRUCT            lpReOpenBuff,
                                          _In_     UINT                  uStyle );
using CreateFileA_pfn = HANDLE (WINAPI *)(_In_     LPCSTR                lpFileName,
                                          _In_     DWORD                 dwDesiredAccess,
                                          _In_     DWORD                 dwShareMode,
                                          _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                          _In_     DWORD                 dwCreationDisposition,
                                          _In_     DWORD                 dwFlagsAndAttributes,
                                          _In_opt_ HANDLE                hTemplateFile);
using CreateFileW_pfn = HANDLE (WINAPI *)(_In_     LPCWSTR               lpFileName,
                                          _In_     DWORD                 dwDesiredAccess,
                                          _In_     DWORD                 dwShareMode,
                                          _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                          _In_     DWORD                 dwCreationDisposition,
                                          _In_     DWORD                 dwFlagsAndAttributes,
                                          _In_opt_ HANDLE                hTemplateFile);

CloseHandle_pfn CloseHandle_Original  = nullptr;
OpenFile_pfn    OpenFile_Original     = nullptr;
CreateFileA_pfn CreateFileA_Original  = nullptr;
CreateFileW_pfn CreateFileW_Original  = nullptr;

volatile HANDLE hTalker_A_SCP = INVALID_HANDLE_VALUE;
volatile HANDLE hTalker_W_SCP = INVALID_HANDLE_VALUE;

HANDLE
WINAPI
SK_YS8_CreateFileA (
  _In_     LPCSTR                lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile )
{
  dwFlagsAndAttributes &= ~FILE_FLAG_RANDOM_ACCESS;
  dwFlagsAndAttributes |=  FILE_FLAG_SEQUENTIAL_SCAN;

  HANDLE hRet =
    CreateFileA_Original ( lpFileName, dwDesiredAccess, dwShareMode,
                             lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
                               hTemplateFile );
  return hRet;
}

HANDLE
WINAPI
SK_YS8_CreateFileW (
  _In_     LPCWSTR               lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile )
{
  dwFlagsAndAttributes &= ~FILE_FLAG_RANDOM_ACCESS;
  dwFlagsAndAttributes |=  FILE_FLAG_SEQUENTIAL_SCAN;

  HANDLE hRet =
    CreateFileW_Original ( lpFileName, dwDesiredAccess, dwShareMode,
                             lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
                               hTemplateFile );

  return hRet;
}

HRESULT
STDMETHODCALLTYPE
SK_YS8_Map (
   _In_ ID3D11DeviceContext      *This,
   _In_ ID3D11Resource           *pResource,
   _In_ UINT                      Subresource,
   _In_ D3D11_MAP                 MapType,
   _In_ UINT                      MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource )
{
  D3D11_MAPPED_SUBRESOURCE local_map = { };

  if (pMappedResource == nullptr)
    pMappedResource = &local_map;

  const HRESULT hr =
    _D3D11_Map_Original ( This, pResource, Subresource,
                            MapType, MapFlags, pMappedResource );

  if (SUCCEEDED (hr))
  {
    if (ManageCleanMemory && pResource && Subresource == 0)
    {
      if (MapType != D3D11_MAP_READ)
      {
        D3D11_RESOURCE_DIMENSION rDim;
            pResource->GetType (&rDim);

        if (rDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D && ys8_dirty_resources.get ()[pResource] == FALSE)
        {
          //dll_log.Log (L"[tid=%x] Map Write  (Clean->Dirty)  <%s>", GetCurrentThreadId (), DescribeResource (pResource).c_str ());

          ys8_dirty_resources.get ()[pResource] = TRUE;
        }
      }
    }
  }

  return hr;
}

extern INT
__stdcall
SK_DXGI_BytesPerPixel (DXGI_FORMAT fmt);

void
STDMETHODCALLTYPE
SK_YS8_CopyResource (
       ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource )
{
  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcResource == nullptr)
  {
    return;
  }

  if (ManageCleanMemory)
  {
    if (ys8_dirty_resources->load_factor () > 3.333)
    {
                                   ys8_dirty_buckets *= 2;
      ys8_dirty_resources->rehash (ys8_dirty_buckets);
    }

    D3D11_RESOURCE_DIMENSION rDim = D3D11_RESOURCE_DIMENSION_UNKNOWN;

    pSrcResource->GetType (&rDim);

    if (rDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      SK_ComQIPtr <ID3D11Texture2D>  pTex2D (pSrcResource);

      D3D11_TEXTURE2D_DESC desc = {};

      pTex2D->GetDesc (&desc);

      const LONG64 llSize =
        SK_DXGI_BytesPerPixel (desc.Format) * desc.Width * desc.Height;

      if (desc.Width == 256 && desc.Height == 128)
      {
        if ((! ys8_dirty_resources->count (pSrcResource)) || ys8_dirty_resources.get ()[pSrcResource] == FALSE)
        {
          InterlockedIncrement (&iCopyOpsSkipped);
          InterlockedAdd64     (&llBytesSkipped, llSize);

          ys8_dirty_resources.get ()[pDstResource] = FALSE;
        //dll_log.Log (L"Skipped CopyResource (256x128) - Not Dirty");
          return;
        }

        else
        {
          InterlockedIncrement (&iCopyOpsRun);
          InterlockedAdd64     (&llBytesCopied, llSize);

          ys8_dirty_resources.get ()[pSrcResource] = FALSE;
          ys8_dirty_resources.get ()[pDstResource] = FALSE;
        }
      }

      else if (desc.Width == 1024 && desc.Height == 64)
      {
        if ((! ys8_dirty_resources->count (pSrcResource)) || ys8_dirty_resources.get ()[pSrcResource] == FALSE)
        {
          InterlockedIncrement (&iCopyOpsSkipped);
          InterlockedAdd64     (&llBytesSkipped, llSize);

          ys8_dirty_resources.get ()[pDstResource] = FALSE;
        //dll_log.Log (L"Skipped CopyResource (1024x64) - Not Dirty");
          return;
        }

        else
        {
          InterlockedIncrement (&iCopyOpsRun);
          InterlockedAdd64     (&llBytesCopied, llSize);

          ys8_dirty_resources.get ()[pSrcResource] = FALSE;
          ys8_dirty_resources.get ()[pDstResource] = FALSE;
        }
      }

      else if (CatchAllMemoryFaults && desc.Usage == D3D11_USAGE_STAGING)
      {
        if ((! ys8_dirty_resources->count (pSrcResource)) || ys8_dirty_resources.get ()[pSrcResource] == FALSE)
        {
          InterlockedIncrement (&iCopyOpsSkipped);
          InterlockedAdd64     (&llBytesSkipped, llSize);

          ys8_dirty_resources.get ()[pDstResource] = FALSE;
        //dll_log.Log (L"Skipped CopyResource (3840x2160) - Not Dirty");
          return;
        }

        else
        {
          // Ignore the video playback
          if (desc.Width != 1920 && desc.Height != 1080)
          {
            InterlockedIncrement (&iCopyOpsRun);
            InterlockedAdd64     (&llBytesCopied, llSize);
          }

          ys8_dirty_resources.get ()[pSrcResource] = FALSE;
          ys8_dirty_resources.get ()[pDstResource] = FALSE;
        }
      }

      //else
      //{
      //  dll_log.Log ( L"[tid=%x] CopyResource:  { %s } -> { %s }", GetCurrentThreadId (),
      //                  DescribeResource (pSrcResource).c_str (),
      //                  DescribeResource (pDstResource).c_str () );
      //}
    }

    //dll_log.Log ( L"CopyResource:  { %s } -> { %s }",
    //                DescribeResource (pSrcResource).c_str (),
    //                DescribeResource (pDstResource).c_str () );
  }

  _D3D11_CopyResource_Original ( This, pDstResource, pSrcResource );
}

__declspec (noinline)
HRESULT
WINAPI
SK_YS8_CreateSamplerState (
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState )
{
  D3D11_SAMPLER_DESC new_desc (*pSamplerDesc);

  //dll_log.Log ( L"CreateSamplerState - Filter: %x, MaxAniso: %lu, MipLODBias: %f, MinLOD: %f, MaxLOD: %f, Comparison: %x, U:%x,V:%x,W:%x - %ws",
  //              new_desc.Filter, new_desc.MaxAnisotropy, new_desc.MipLODBias, new_desc.MinLOD, new_desc.MaxLOD,
  //              new_desc.ComparisonFunc, new_desc.AddressU, new_desc.AddressV, new_desc.AddressW, SK_SummarizeCaller ().c_str () );

  if (new_desc.Filter <= D3D11_FILTER_ANISOTROPIC)
  {
    if (new_desc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT)
    {
      new_desc.Filter        = anisotropic_filter ? D3D11_FILTER_ANISOTROPIC :
                                                    D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      new_desc.MaxAnisotropy = anisotropic_filter ? max_anisotropy : 1;

      if (disable_negative_lod_bias && new_desc.MipLODBias < 0.0f)
                                       new_desc.MipLODBias = 0.0f;

      if (explicit_lod_bias != 0.0f)
      {
        new_desc.MipLODBias = explicit_lod_bias;
      }

      if (nearest_neighbor)
      {
        new_desc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
      }

      if ( new_desc.ComparisonFunc > D3D11_COMPARISON_NEVER &&
           new_desc.ComparisonFunc < D3D11_COMPARISON_ALWAYS )
      {
        new_desc.Filter = anisotropic_filter ? D3D11_FILTER_COMPARISON_ANISOTROPIC :
                                               D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
      }

      pSamplerDesc = &new_desc;
    }
  }

  return
    _D3D11Dev_CreateSamplerState_Original ( This, pSamplerDesc,
                                              ppSamplerState );
}

__declspec (noinline)
HRESULT
WINAPI
SK_YS8_CreateShaderResourceView (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView )
{
  return
    _D3D11Dev_CreateShaderResourceView_Original ( This, pResource, pDesc, ppSRView );
}

__declspec (noinline)
HRESULT
WINAPI
SK_YS8_CreateTexture2D (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D)
{
  bool shadow_tex = false;

  if (ppTexture2D == nullptr || pDesc == nullptr)
  {
    return _D3D11Dev_CreateTexture2D_Original ( This, pDesc,
                                                  pInitialData, ppTexture2D );
  }

  D3D11_TEXTURE2D_DESC newDesc (*pDesc);

  const bool depth_format =
    pDesc->Format == DXGI_FORMAT_R24G8_TYPELESS    ||
    pDesc->Format == DXGI_FORMAT_R32G8X24_TYPELESS ||
    pDesc->Format == DXGI_FORMAT_B8G8R8X8_UNORM    || (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) ==
                                                                          D3D11_BIND_DEPTH_STENCIL;

  const bool composite_format = (
    pDesc->Format == DXGI_FORMAT_B8G8R8A8_UNORM
  );

  if ( pInitialData == nullptr )
  {
    //if (pDesc->Width == 4096 && pDesc->Height == 2160)
    //{
    //  newDesc.Width  = 3840;
    //  newDesc.Height = 2160;
    //}
    //
    //else if (pDesc->Width == 2048 && pDesc->Height == 1080)
    //{
    //  newDesc.Width  = 1920;
    //  newDesc.Height = 1080;
    //}
    //
    //else if (pDesc->Width == 2048 && pDesc->Height == 720)
    //{
    //  newDesc.Width  = 1280;
    //  newDesc.Height = 720;
    //}


    //extern std::wstring
    //__stdcall
    //SK_DXGI_FormatToStr (DXGI_FORMAT fmt);
    //
    //dll_log.Log (L"YS8: %lux%lu - Bind Flags: %s, CPU Usage: %x, Format: %s",
    //                                                               pDesc->Width, pDesc->Height,
    //                    SK_D3D11_DescribeBindFlags ((D3D11_BIND_FLAG)pDesc->BindFlags).c_str (),
    //                                                                 pDesc->CPUAccessFlags,
    //                                           SK_DXGI_FormatToStr  (pDesc->Format).c_str    () );

    if ( pDesc->Width == 4096 && pDesc->Height == 4096 && ( depth_format || composite_format ) && ( pDesc->Usage != D3D11_USAGE_STAGING ) )
    {
      ///SK_LOG0 ( ( L"Ys8 Shadow(?) Origin: '%s' -- Format: %lu, Misc Flags: %x, MipLevels: %lu, "
      ///            L"ArraySize: %lu, CPUAccess: %x, BindFlags: %x, Usage: %x",
      ///              SK_GetCallerName ().c_str (), pDesc->Format,
      ///                pDesc->MiscFlags,      pDesc->MipLevels, pDesc->ArraySize,
      ///                pDesc->CPUAccessFlags, pDesc->BindFlags, pDesc->Usage
      ///          ),
      ///          L"DX11TexMgr" );

      newDesc.Width  = static_cast <UINT> ( static_cast <double> (newDesc.Width)  * __SK_YS8_ShadowScale );
      newDesc.Height = static_cast <UINT> ( static_cast <double> (newDesc.Height) * __SK_YS8_ShadowScale );

      shadow_tex = true;
    }

    pDesc = &newDesc;
  }

  SK_TLS *pTLS = SK_TLS_Bottom ();

  SK_ScopedBool auto_bool (&pTLS->texture_management.injection_thread);
  if (shadow_tex)           pTLS->texture_management.injection_thread = true;

  const HRESULT hr =
    _D3D11Dev_CreateTexture2D_Original ( This,
                                  pDesc, pInitialData,
                                         ppTexture2D );

  if (SUCCEEDED (hr))
  {
    if (shadow_tex)
    {
      //shadowmap_textures.emplace (ppTexture2D);
    }
  }

  return hr;
}


void
SK_YS8_InitPlugin (void)
{
  SK_SetPluginName (YS8_VERSION_STR);

  SK_CreateFuncHook (       L"ID3D11Device::CreateTexture2D",
                               D3D11Dev_CreateTexture2D_Override,
                                 SK_YS8_CreateTexture2D,
     static_cast_p2p <void> (&_D3D11Dev_CreateTexture2D_Original) );
  MH_QueueEnableHook (         D3D11Dev_CreateTexture2D_Override  );

  SK_CreateFuncHook (       L"ID3D11DeviceContext::Map",
                                  D3D11_Map_Override,
                                 SK_YS8_Map,
        static_cast_p2p <void> (&_D3D11_Map_Original) );
  MH_QueueEnableHook (            D3D11_Map_Override  );

  SK_CreateFuncHook (       L"ID3D11DeviceContext::CopyResource",
                                  D3D11_CopyResource_Override,
                                 SK_YS8_CopyResource,
        static_cast_p2p <void> (&_D3D11_CopyResource_Original) );
  MH_QueueEnableHook (            D3D11_CopyResource_Override  );

  //SK_CreateFuncHook (       L"ID3D11Device::CreateShaderResourceView",
  //                             D3D11Dev_CreateShaderResourceView_Override,
  //                               SK_YS8_CreateShaderResourceView,
  //   static_cast_p2p <void> (&_D3D11Dev_CreateShaderResourceView_Original) );
  //MH_QueueEnableHook (         D3D11Dev_CreateShaderResourceView_Override  );

  SK_CreateFuncHook (       L"ID3D11Device::CreateSamplerState",
                               D3D11Dev_CreateSamplerState_Override,
                                 SK_YS8_CreateSamplerState,
     static_cast_p2p <void> (&_D3D11Dev_CreateSamplerState_Original) );
  MH_QueueEnableHook (         D3D11Dev_CreateSamplerState_Override  );

  SK_CreateFuncHook (       L"ID3D11DeviceContext::RSSetViewports",
                               D3D11_RSSetViewports_Override,
                                 SK_YS8_RSSetViewports,
     static_cast_p2p <void> (&_D3D11_RSSetViewports_Original) );
  MH_QueueEnableHook (         D3D11_RSSetViewports_Override  );

  SK_CreateFuncHook (       L"SK_PlugIn_ControlPanelWidget",
                              SK_PlugIn_ControlPanelWidget,
                                 SK_YS8_ControlPanel,
     static_cast_p2p <void> (&SK_PlugIn_ControlPanelWidget_Original) );
  MH_QueueEnableHook (        SK_PlugIn_ControlPanelWidget           );



  SK_CreateDLLHook2 (               L"kernel32",
                                     "CreateFileA",
                               SK_YS8_CreateFileA,
             static_cast_p2p <void> (&CreateFileA_Original) );
  SK_CreateDLLHook2 (               L"kernel32",
                                     "CreateFileW",
                               SK_YS8_CreateFileW,
             static_cast_p2p <void> (&CreateFileW_Original) );


  auto&
    ys8_cfg = ys8_config.get ();

  ys8_cfg.shadows.scale =
      dynamic_cast <sk::ParameterFloat *>
        (g_ParameterFactory->create_parameter <float> (L"Shadow Rescale"));

  ys8_cfg.shadows.scale->register_to_ini ( SK_GetDLLConfig (),
                                            L"Ys8.Shadows",
                                              L"Scale" );

  ys8_cfg.shadows.scale->load (__SK_YS8_ShadowScale);

  __SK_YS8_ShadowScale =
    std::fmax (  1.f,
     std::fmin ( 4.f, __SK_YS8_ShadowScale )
              );

  ys8_cfg.style.pixel_style =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory->create_parameter <std::wstring> (L"Art Style"));

  ys8_cfg.style.pixel_style->register_to_ini ( SK_GetDLLConfig (),
                                                L"Ys8.Style",
                                                  L"PixelStyle" );

  std::wstring style;
  ys8_cfg.style.pixel_style->load (style);

  bool aa = true;

  //if (! SK_IsInjected ())
  {
    if (0 == _wcsicmp (style.c_str (), L"Retro"))
    {
      max_anisotropy            = 8;
      anisotropic_filter        = true;

      disable_negative_lod_bias =     TRUE;
      explicit_lod_bias         = 3.16285f;
      nearest_neighbor          =     TRUE;

      b_66b35959 = true;
      b_9d665ae2 = false;
      b_b21c8ab9 = false;
      b_6bb0972d = true;
      b_05da09bd = false;

      aa = false;
    }

    else if (0 == _wcsicmp (style.c_str (), L"Sharp"))
    {
      max_anisotropy            =   14;
      anisotropic_filter        = true;

      disable_negative_lod_bias =   FALSE;
      explicit_lod_bias         =   -3.0f;
      nearest_neighbor          =   FALSE;

      b_66b35959 = false;
      b_9d665ae2 = false;
      b_b21c8ab9 = false;
      b_6bb0972d = false;
      b_05da09bd = true;

      aa = false;
    }
  }

  if (aa)
  {
    max_anisotropy            =   16;
    anisotropic_filter        = true;

    disable_negative_lod_bias =   TRUE;
    explicit_lod_bias         =   0.0f;
    nearest_neighbor          =  FALSE;

    b_66b35959 = false;
    b_9d665ae2 = false;
    b_b21c8ab9 = false;
    b_6bb0972d = false;
    b_05da09bd = false;
  }





  ys8_cfg.mipmaps.cache =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Store Completely Mipmapped Textures"));

  ys8_cfg.mipmaps.cache->register_to_ini ( SK_GetDLLConfig (),
                                            L"Ys8.Mipmaps",
                                              L"CacheToDisk" );
  ys8_cfg.mipmaps.cache->load (config.textures.d3d11.cache_gen_mips);

  ys8_cfg.mipmaps.stream_rate =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory->create_parameter <int> (L"Streaming Priority"));

  ys8_cfg.mipmaps.stream_rate->register_to_ini ( SK_GetDLLConfig (),
                                                  L"Ys8.Mipmaps",
                                                    L"StreamingPriority" );

  ys8_cfg.mipmaps.anisotropy =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory->create_parameter <int> (L"Maximum Anisotropic Filtering Level"));

  ys8_cfg.mipmaps.anisotropy->register_to_ini ( SK_GetDLLConfig (),
                                                 L"Ys8.Mipmaps",
                                                   L"MaxAnisotropy" );

  ys8_cfg.mipmaps.anisotropy->load (max_anisotropy);


  ys8_cfg.performance.manage_clean_memory =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Take control of redundant buffer uploads"));

  ys8_cfg.performance.manage_clean_memory->register_to_ini ( SK_GetDLLConfig (),
                                                              L"Ys8.Memory",
                                                                L"SkipUnchangedTextUploads" );

  ys8_cfg.performance.manage_clean_memory->load (ManageCleanMemory);

  ys8_cfg.performance.aggressive_memory_mgmt =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Try to prevent even more"));

  ys8_cfg.performance.aggressive_memory_mgmt->register_to_ini ( SK_GetDLLConfig (),
                                                                 L"Ys8.Memory",
                                                                   L"AggressiveMode" );

  ys8_cfg.performance.aggressive_memory_mgmt->load (CatchAllMemoryFaults);



  ys8_cfg.postproc.override_postproc =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Override post-processing"));

  ys8_cfg.postproc.contrast =
      dynamic_cast <sk::ParameterFloat *>
        (g_ParameterFactory->create_parameter <float> (L"Contrast"));

  ys8_cfg.postproc.sharpness =
      dynamic_cast <sk::ParameterFloat *>
        (g_ParameterFactory->create_parameter <float> (L"Sharpness"));

  ys8_cfg.postproc.vignette =
      dynamic_cast <sk::ParameterFloat *>
        (g_ParameterFactory->create_parameter <float> (L"Vignette"));


  ys8_cfg.postproc.override_postproc->register_to_ini ( SK_GetDLLConfig (),
                                                         L"Ys8.PostProcess",
                                                           L"Override" );

  ys8_cfg.postproc.contrast->register_to_ini ( SK_GetDLLConfig (),
                                                L"Ys8.PostProcess",
                                                  L"Contrast" );
  ys8_cfg.postproc.vignette->register_to_ini ( SK_GetDLLConfig (),
                                                L"Ys8.PostProcess",
                                                  L"Vignette" );
  ys8_cfg.postproc.sharpness->register_to_ini ( SK_GetDLLConfig (),
                                                 L"Ys8.PostProcess",
                                                   L"Sharpness" );


  __SK_D3D11_PixelShader_CBuffer_Overrides->push_back (
    { 0x05da09bd, 48, false, 0, 0, 48, { 0.0f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f } }
  );

  SK_YS8_CB_Override = &__SK_D3D11_PixelShader_CBuffer_Overrides->back ();

  ys8_cfg.postproc.override_postproc->load (SK_YS8_CB_Override->Enable);
  ys8_cfg.postproc.contrast->load          (SK_YS8_CB_Override->Values [0]);
  ys8_cfg.postproc.vignette->load          (SK_YS8_CB_Override->Values [1]);
  ys8_cfg.postproc.sharpness->load         (SK_YS8_CB_Override->Values [3]);

  SK_ApplyQueuedHooks ();

  InterlockedExchange (&__YS8_init, 1);
};